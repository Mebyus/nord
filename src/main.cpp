#include <ctype.h>
#include <errno.h>
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

#define CTRL_KEY(k) ((k) & 0x1f)

struct termios original_terminal_state;

fn void fatal(const char* s) noexcept;

fn internal void exit_raw_mode() noexcept {
  i32 rcode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_state);
  if (rcode < 0) {
    fatal("failed to exit raw mode\r\n");
  }
}

fn internal void enter_raw_mode() noexcept {
  i32 rcode = tcgetattr(STDIN_FILENO, &original_terminal_state);
  if (rcode < 0) {
    fatal("failed to get current terminal state\n");
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
    fatal("failed to enter raw mode\n");
  }
  atexit(exit_raw_mode);
}

fn internal struct winsize get_viewport_size() noexcept {
  struct winsize ws;
  i32 rcode = ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  if (rcode < 0 || ws.ws_col == 0) {
    fatal("failed to get viewport size\r\n");
  }

  return ws;
}

fn internal void stdout_write(mc c) noexcept {
  const i32 stdout_fd = 1;

  var u8* ptr = c.ptr;
  var usz len = c.len;
  while (len > 0) {
    var isz n = write(stdout_fd, ptr, len);
    if (n < 0) {
      fatal("failed to write to stdout\r\n");
    }

    len -= cast(usz, n);
    ptr += n;
  }
}

struct CommandBuffer {
  bb s;

  con CommandBuffer() noexcept {}

  con CommandBuffer(usz initial_size) noexcept {
    var u8* bytes = (u8*)malloc(initial_size);
    if (bytes == nil) {
      fatal("failed to allocate memory for command buffer\r\n");
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
      fatal("failed to grow memory for command buffer\r\n");
    }
    s.cap = new_cap;
    s.ptr = bytes;

    s.unsafe_write(c);
  }

  method void reset() noexcept { s.reset(); }

  method void flush() noexcept {
    stdout_write(s.chunk());
    reset();
  }
};

#define COMMAND_BUFFER_INITIAL_SIZE 1 << 14

struct Editor {
  CommandBuffer cmd_buf;

  u32 rows_num;
  u32 cols_num;

  con Editor() noexcept {}

  method void init() noexcept {
    enter_raw_mode();

    cmd_buf = CommandBuffer(COMMAND_BUFFER_INITIAL_SIZE);

    clear_window();

    struct winsize ws = get_viewport_size();
    rows_num = cast(u32, ws.ws_row);
    cols_num = cast(u32, ws.ws_col);

    draw_test();
  }

  method void draw_test() noexcept {
    u32 y;
    for (y = 0; y < rows_num - 1; y += 1) {
      cmd_buf.write(str("#\r\n"));
    }
    cmd_buf.write(str("#"));

    cmd_buf.flush();
  }

  method void clear_window() noexcept {
    cmd_buf.reset();

    cmd_buf.write(str("\x1b[2J"));
    cmd_buf.write(str("\x1b[H"));

    cmd_buf.flush();
  }
};

var Editor e;

fn void fatal(const char* s) noexcept {
  e.clear_window();

  perror(s);
  exit(1);
}

fn internal u8 read_key_input() noexcept {
  while (true) {
    u8 c = 0;
    isz num_bytes_read = read(STDIN_FILENO, &c, 1);
    if ((num_bytes_read == -1 && errno != EAGAIN) || num_bytes_read == 0) {
      // read timed out, no input was given
      continue;
    }

    if (num_bytes_read < 0) {
      fatal("failed to read input\r\n");
    }

    return c;
  }

  fatal("unreachable [0x0002]\n");
  return 0;
}

fn internal void handle_key_input(u8 c) noexcept {
  if (c == CTRL_KEY('q')) {
    e.clear_window();
    exit(0);
  }
}

fn i32 main() noexcept {
  e.init();

  while (true) {
    u8 c = read_key_input();
    handle_key_input(c);
  }

  fatal("unreachable [0x0001]\n");
  return 0;
}
