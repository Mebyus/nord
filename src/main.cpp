#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

#include "core.hpp"

fn void copy(u8* src, u8* dst, usz n) noexcept {
  if (n <= 16) {
    for (usz i = 0; i < n; i += 1) {
      dst[i] = src[i];
    }
    return;
  }
  memcpy(dst, src, n);
}

fn internal void fd_write(i32 fd, mc c) noexcept {
  var u8* ptr = c.ptr;
  var usz len = c.len;
  while (len > 0) {
    var isz n = write(fd, ptr, len);
    if (n < 0) {
      // TODO: add error handling
      return;
    }

    len -= cast(usz, n);
    ptr += n;
  }
}

fn internal void stdout_write(mc c) noexcept {
  const i32 stdout_fd = 1;
  fd_write(stdout_fd, c);
}

fn internal void stderr_write(mc c) noexcept {
  const i32 stderr_fd = 2;
  fd_write(stderr_fd, c);
}

#define CTRL_KEY(k) ((k) & 0x1f)

var global struct termios original_terminal_state;

#define MAX_BACKTRACE_LEVEL 32
#define PANIC_DISPLAY_BUFFER_SIZE 1 << 10

var global usz backtrace_buffer[MAX_BACKTRACE_LEVEL];
var global u8 panic_display_buffer[PANIC_DISPLAY_BUFFER_SIZE];

fn void fatal(mc msg, mc filename, u32 line) noexcept {
  var bb buf = bb(panic_display_buffer, PANIC_DISPLAY_BUFFER_SIZE);

  buf.write(filename);
  buf.write(macro_static_str(":"));
  buf.format_dec(line);

  buf.write(macro_static_str("\npanic: "));
  buf.write(msg);
  buf.write(macro_static_str("\n"));

  stderr_write(buf.chunk());

  i32 number_of_entries =
      backtrace(cast(void**, backtrace_buffer), MAX_BACKTRACE_LEVEL);

  const i32 stderr_fd = 2;
  backtrace_symbols_fd(cast(void**, &backtrace_buffer), number_of_entries,
                       stderr_fd);

  exit(1);
}

#define panic(msg) fatal(msg, macro_src_file, macro_src_line)

fn internal void exit_raw_mode() noexcept {
  i32 rcode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_state);
  if (rcode < 0) {
    panic(macro_static_str("failed to exit raw mode"));
  }
}

fn internal void enter_raw_mode() noexcept {
  i32 rcode = tcgetattr(STDIN_FILENO, &original_terminal_state);
  if (rcode < 0) {
    panic(macro_static_str("failed to get current terminal state"));
  }

  struct termios terminal_state = original_terminal_state;
  terminal_state.c_cflag |= (cast(u32, CS8));
  terminal_state.c_oflag &= ~(cast(u32, OPOST));
  terminal_state.c_iflag &=
      ~(cast(u32, BRKINT) | cast(u32, INPCK) | cast(u32, ISTRIP) |
        cast(u32, ICRNL) | cast(u32, IXON));
  terminal_state.c_lflag &= ~(cast(u32, ECHO) | cast(u32, ICANON) |
                              cast(u32, IEXTEN) | cast(u32, ISIG));
  terminal_state.c_cc[VMIN] = 0;
  terminal_state.c_cc[VTIME] = 1;

  rcode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal_state);
  if (rcode < 0) {
    panic(macro_static_str("failed to enter raw mode"));
  }
  atexit(exit_raw_mode);
}

fn internal struct winsize get_viewport_size() noexcept {
  struct winsize ws;
  i32 rcode = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  if (rcode < 0 || ws.ws_col == 0) {
    panic(macro_static_str("failed to get viewport size"));
  }

  return ws;
}

#define COMMAND_SKETCH_BUFFER_SIZE 32

struct CommandBuffer {
  u8 sketch_buf[COMMAND_SKETCH_BUFFER_SIZE];

  bb s;

  con CommandBuffer() noexcept {}

  con CommandBuffer(usz initial_size) noexcept {
    var u8* bytes = (u8*)malloc(initial_size);
    if (bytes == nil) {
      panic(macro_static_str("failed to allocate memory for command buffer"));
    }

    s = bb(bytes, initial_size);
  }

  method void write(mc c) noexcept {
    if (s.rem() >= c.len) {
      s.unsafe_write(c);
      return;
    }

    // grow buffer capacity
    var dirty usz new_cap;
    if (c.len <= s.cap) {
      new_cap = s.cap << 1;
    } else {
      new_cap = s.cap + c.len + (s.cap >> 1);
    }

    var u8* bytes = (u8*)realloc(s.ptr, new_cap);
    if (bytes == nil) {
      panic(macro_static_str("failed to grow memory for command buffer"));
    }
    s.cap = new_cap;
    s.ptr = bytes;

    s.unsafe_write(c);
  }

  method void change_cursor_position(u32 x, u32 y) noexcept {
    // prepare command string
    bb buf = bb(sketch_buf, COMMAND_SKETCH_BUFFER_SIZE);
    buf.unsafe_write(macro_static_str("\x1b["));
    buf.format_dec(y + 1);
    buf.unsafe_write(cast(u8, ';'));
    buf.format_dec(x + 1);
    buf.unsafe_write(cast(u8, 'H'));

    // write prepared command to buffer
    write(buf.chunk());
  }

  method void hide_cursor() noexcept { write(macro_static_str("\x1b[?25l")); }

  method void show_cursor() noexcept { write(macro_static_str("\x1b[?25h")); }

  // Begin new line in output
  method void nl() noexcept { write(macro_static_str("\r\n")); }

  method void reset() noexcept { s.reset(); }

  method void flush() noexcept {
    stdout_write(s.chunk());
    reset();
  }
};

#define COMMAND_BUFFER_INITIAL_SIZE 1 << 14

struct TextLine {
  // line number, zero-based
  u32 num;

  mc data;
};

var global TextLine lines_buf[2];

struct Editor {
  // type of input sequence
  enum struct Seq : u8 {
    // key is a regular printable character
    REGULAR,

    // malformed sequence
    UNKNOWN,

    ESC,

    LEFT,
    RIGHT,
    UP,
    DOWN,

    PAGE_UP,
    PAGE_DOWN,

    DELETE,
  };

  struct Key {
    // regular character
    u8 c;

    Seq s;
  };

  CommandBuffer cmd_buf;

  chunk<TextLine> lines;

  // cursor position, originating from top-left corner
  u32 cx;
  u32 cy;

  // terminal screen sizes
  u32 rows_num;
  u32 cols_num;

  con Editor() noexcept {}

  method void init() noexcept {
    cx = 0;
    cy = 0;

    enter_raw_mode();

    cmd_buf = CommandBuffer(COMMAND_BUFFER_INITIAL_SIZE);

    clear_window();

    struct winsize ws = get_viewport_size();
    rows_num = cast(u32, ws.ws_row);
    cols_num = cast(u32, ws.ws_col);

    // test data
    lines = chunk(lines_buf, 2);

    lines.ptr[0].num = 0;
    lines.ptr[0].data = macro_static_str("Hello, world!");

    lines.ptr[1].num = 1;
    lines.ptr[1].data = macro_static_str(
        "Hello, world! Hello, world! Hello, world! Hello, world! Hello, "
        "world! Hello, world! Hello, world! Hello, world! Hello, world! "
        "Hello, world!");
    // end test data

    cmd_buf.hide_cursor();
    draw_test();
    cmd_buf.show_cursor();
    update_cursor_position();
    cmd_buf.flush();
  }

  method void init(cstr filename) noexcept {
    stdout_write(filename.as_str());
  }

  method void draw_test() noexcept {
    u32 y = 0;
    for (; y < lines.len && y < rows_num - 1; y += 1) {
      cmd_buf.write(lines.ptr[y].data.crop(cols_num));
      cmd_buf.nl();
    }

    for (; y < rows_num - 1; y += 1) {
      cmd_buf.write(macro_static_str("#\r\n"));
    }
    cmd_buf.write(macro_static_str("#"));

    var dirty u8 s[32];
    var bb buf = bb(s, 32);
    buf.write(macro_static_str("   cols = "));
    buf.format_dec(cols_num);
    buf.write(macro_static_str(", rows = "));
    buf.format_dec(rows_num);
    cmd_buf.write(buf.chunk());
  }

  method void move_cursor_right() noexcept {
    if (cx >= cols_num - 1) {
      return;
    }
    cx += 1;
  }

  method void move_cursor_left() noexcept {
    if (cx == 0) {
      return;
    }
    cx -= 1;
  }

  method void move_cursor_up() noexcept {
    if (cy == 0) {
      return;
    }
    cy -= 1;
  }

  method void move_cursor_down() noexcept {
    if (cy >= rows_num - 1) {
      return;
    }
    cy += 1;
  }

  method void move_cursor_top() noexcept { cy = 0; }

  method void move_cursor_bot() noexcept { cy = rows_num - 1; }

  method void update_cursor_position() noexcept {
    cmd_buf.change_cursor_position(cx, cy);
  }

  method void update_window() noexcept {
    update_cursor_position();

    cmd_buf.flush();
  }

  method void clear_window() noexcept {
    cmd_buf.reset();

    cmd_buf.write(macro_static_str("\x1b[2J"));  // clear terminal screen
    cmd_buf.write(
        macro_static_str("\x1b[H"));  // position cursor at the top-left corner

    cmd_buf.flush();
  }
};

var global Editor e;

fn internal Editor::Key read_key_input() noexcept {
  const i32 stdin_fd = 0;

  u8 c = 0;
  while (true) {
    isz num_bytes_read = read(stdin_fd, &c, 1);
    if ((num_bytes_read == -1 && errno != EAGAIN) || num_bytes_read == 0) {
      // read timed out, no input was given
      continue;
    }

    if (num_bytes_read < 0) {
      panic(macro_static_str("failed to read input"));
    }

    break;
  }

  if (c != '\x1b') {
    return Editor::Key{.c = c, .s = Editor::Seq::REGULAR};
  }

  u8 seq[3];

  isz num_bytes_read = read(stdin_fd, &seq[0], 1);
  if (num_bytes_read != 1) {
    return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
  }

  num_bytes_read = read(stdin_fd, &seq[1], 1);
  if (num_bytes_read != 1) {
    return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
  }

  if (seq[0] == '[') {
    if ('0' <= seq[1] && seq[1] <= '9') {
      num_bytes_read = read(stdin_fd, &seq[2], 1);
      if (num_bytes_read != 1) {
        return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
      }

      if (seq[2] == '~') {
        switch (seq[1]) {
          case '3': {
            return Editor::Key{.c = 0, .s = Editor::Seq::DELETE};
          }
          case '5': {
            return Editor::Key{.c = 0, .s = Editor::Seq::PAGE_UP};
          }
          case '6': {
            return Editor::Key{.c = 0, .s = Editor::Seq::PAGE_DOWN};
          }
          default: {
            return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
          }
        }
      }
    }

    switch (seq[1]) {
      case 'A': {
        return Editor::Key{.c = 0, .s = Editor::Seq::UP};
      }
      case 'B': {
        return Editor::Key{.c = 0, .s = Editor::Seq::DOWN};
      }
      case 'C': {
        return Editor::Key{.c = 0, .s = Editor::Seq::RIGHT};
      }
      case 'D': {
        return Editor::Key{.c = 0, .s = Editor::Seq::LEFT};
      }
      default: {
        return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
      }
    }
  }

  return Editor::Key{.c = 0, .s = Editor::Seq::UNKNOWN};
}

fn internal void handle_key_input(Editor::Key k) noexcept {
  if (k.s == Editor::Seq::REGULAR) {
    if (k.c == CTRL_KEY('q')) {
      e.clear_window();
      exit(0);
    }
    return;
  }

  switch (k.s) {
    case Editor::Seq::LEFT: {
      e.move_cursor_left();
      break;
    }
    case Editor::Seq::RIGHT: {
      e.move_cursor_right();
      break;
    }
    case Editor::Seq::UP: {
      e.move_cursor_up();
      break;
    }
    case Editor::Seq::DOWN: {
      e.move_cursor_down();
      break;
    }

    case Editor::Seq::PAGE_UP: {
      e.move_cursor_top();
      break;
    }
    case Editor::Seq::PAGE_DOWN: {
      e.move_cursor_bot();
      break;
    }

    case Editor::Seq::DELETE: {
      e.move_cursor_top();
      break;
    }

    case Editor::Seq::UNKNOWN: {
      break;
    }
    case Editor::Seq::ESC: {
      break;
    }
    case Editor::Seq::REGULAR: {
      panic(macro_static_str("unreachable"));
    }
    default: {
      break;
    }
  }

  e.update_window();
}

fn i32 main(i32 argc, u8** argv) noexcept {
  if (argc < 2) {
    e.init();
  } else {
    cstr filename = cstr(argv[1]);
    e.init(filename);
    return 0; // TODO: remove this stub
  }

  while (true) {
    Editor::Key k = read_key_input();
    handle_key_input(k);
  }

  panic(macro_static_str("unreachable"));
  return 0;
}
