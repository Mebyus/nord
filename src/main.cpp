#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
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

method void mc::clear() noexcept {
  if (len <= 16) {
    for (usz i = 0; i < len; i += 1) {
      ptr[i] = 0;
    }
    return;
  }

  memset(ptr, 0, len);
}

fn internal void fd_write(i32 fd, mc c) noexcept {
  var isz n = write(fd, c.ptr, c.len);
  if (n < 0) {
    // TODO: add error handling
    return;
  }
}

fn void stdout_write(mc c) noexcept {
  const i32 stdout_fd = 1;
  fd_write(stdout_fd, c);
}

fn void stderr_write(mc c) noexcept {
  const i32 stderr_fd = 2;
  fd_write(stderr_fd, c);
}

fn internal void fd_write_all(i32 fd, mc c) noexcept {
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

// Returns number of bytes read from file descriptor
fn internal usz fd_read_all(i32 fd, mc c) noexcept {
  var bb buf = bb(c);
  while (!buf.is_full()) {
    var isz n = read(fd, buf.tip(), buf.rem());
    if (n <= 0) {
      return buf.len;
    }
    buf.len += cast(usz, n);
  }
  return buf.len;
}

fn void stdout_write_all(mc c) noexcept {
  const i32 stdout_fd = 1;
  fd_write_all(stdout_fd, c);
}

fn void stderr_write_all(mc c) noexcept {
  const i32 stderr_fd = 2;
  fd_write_all(stderr_fd, c);
}

fn FileReadResult read_file(cstr filename) noexcept {
  var dirty struct stat s;
  var i32 rcode = stat(cast(char*, filename.ptr), &s);
  if (rcode < 0) {
    return FileReadResult(FileReadResult::Code::Error);
  }

  const usz size = s.st_size;
  if (size == 0) {
    // File is empty, nothing to read
    return FileReadResult(mc());
  }

  const i32 fd = open(cast(char*, filename.ptr), O_RDONLY);
  if (fd < 0) {
    return FileReadResult(FileReadResult::Code::Error);
  }

  var mc data = mem::alloc(size);
  if (data.is_nil()) {
    close(fd);
    return FileReadResult(FileReadResult::Code::Error);
  }

  const usz n = fd_read_all(fd, data);
  close(fd);
  if (n == 0) {
    mem::free(data);
    return FileReadResult(FileReadResult::Code::Error);
  }

  return FileReadResult(data.slice_to(n));
}

#define CTRL_KEY(k) ((k) & 0x1f)

var global struct termios original_terminal_state;

#define MAX_BACKTRACE_LEVEL 32
#define PANIC_DISPLAY_BUFFER_SIZE 1 << 10

var global usz backtrace_buffer[MAX_BACKTRACE_LEVEL];
var global u8 panic_display_buffer[PANIC_DISPLAY_BUFFER_SIZE];

fn void fatal(mc msg, src_loc loc) noexcept {
  var bb buf = bb(panic_display_buffer, PANIC_DISPLAY_BUFFER_SIZE);

  buf.write(loc.file);
  buf.write(macro_static_str(":"));
  buf.fmt_dec(loc.line);

  buf.write(macro_static_str("\npanic: "));
  buf.write(msg);
  buf.write(macro_static_str("\n"));

  stderr_write_all(buf.head());

  i32 number_of_entries =
      backtrace(cast(void**, backtrace_buffer), MAX_BACKTRACE_LEVEL);

  const i32 stderr_fd = 2;
  backtrace_symbols_fd(cast(void**, &backtrace_buffer), number_of_entries,
                       stderr_fd);

  abort();
}

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

struct Color {
  u8 red;
  u8 green;
  u8 blue;

  let Color(u8 r, u8 g, u8 b) noexcept : red(r), green(g), blue(b) {}
};

internal const usz terminal_sketch_buffer_size = 32;

struct TerminalOutputBuffer {
  u8 sketch_buf[terminal_sketch_buffer_size];

  DynBytesBuffer db;

  let TerminalOutputBuffer() noexcept {}

  let TerminalOutputBuffer(usz initial_size) noexcept {
    db = DynBytesBuffer(initial_size);
  }

  method void write(mc c) noexcept { db.write(c); }

  method void change_cursor_position(u32 x, u32 y) noexcept {
    // prepare command string
    var bb buf = bb(sketch_buf, terminal_sketch_buffer_size);

    buf.unsafe_write(macro_static_str("\x1b["));
    buf.fmt_dec(y + 1);
    buf.unsafe_write(';');
    buf.fmt_dec(x + 1);
    buf.unsafe_write('H');

    // write prepared command to buffer
    write(buf.head());
  }

  method void set_text_color(Color color) noexcept {
    var bb buf = bb(sketch_buf, terminal_sketch_buffer_size);

    buf.write(macro_static_str("\x1b[38;2;"));
    buf.fmt_dec(color.red);
    buf.unsafe_write(';');
    buf.fmt_dec(color.green);
    buf.unsafe_write(';');
    buf.fmt_dec(color.blue);
    buf.unsafe_write('m');

    write(buf.head());
  }

  method void set_background_color(Color color) noexcept {
    var bb buf = bb(sketch_buf, terminal_sketch_buffer_size);

    buf.write(macro_static_str("\x1b[48;2;"));
    buf.fmt_dec(color.red);
    buf.unsafe_write(';');
    buf.fmt_dec(color.green);
    buf.unsafe_write(';');
    buf.fmt_dec(color.blue);
    buf.unsafe_write('m');

    write(buf.head());
  }

  method void hide_cursor() noexcept { write(macro_static_str("\x1b[?25l")); }

  method void show_cursor() noexcept { write(macro_static_str("\x1b[?25h")); }

  // Begin new line in output
  method void nl() noexcept { write(macro_static_str("\r\n")); }

  method void reset() noexcept { db.reset(); }

  method void flush() noexcept {
    stdout_write_all(db.head());
    reset();
  }
};

#define COMMAND_BUFFER_INITIAL_SIZE 1 << 14

struct Token {
  enum struct Kind : u8 {
    EMPTY = 0,
    EOL,
    DIRECTIVE,
    KEYWORD_GROUP_1,
    KEYWORD_GROUP_2,
    BUILTIN,
    IDENTIFIER,
    STRING,
    CHARACTER,
    COMMENT,
    NUMBER,
    OPERATOR,
    SPACE,
    TAB,
  };

  str lit;
  Token::Kind kind;

  // true for Tokens for which visual representation is not
  // the same as their byte sequence in literal
  bool is_indirect;

  let Token() noexcept : lit(mc()), kind(Kind::EMPTY), is_indirect(false) {}

  let Token(Token::Kind k) noexcept : lit(mc()), kind(k), is_indirect(false) {}

  let Token(Token::Kind k, str l) noexcept
      : lit(l), kind(k), is_indirect(false) {}
};

internal const Token static_literals_table[] = {
    Token(Token::Kind::DIRECTIVE, macro_static_str("#define")),
    Token(Token::Kind::DIRECTIVE, macro_static_str("#include")),
    Token(Token::Kind::DIRECTIVE, macro_static_str("#ifndef")),
    Token(Token::Kind::DIRECTIVE, macro_static_str("#undef")),

    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("var")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("const")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("struct")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("enum")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("fn")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("method")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("let")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("return")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("switch")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("if")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("else")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("while")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("namespace")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("typedef")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("case")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("default")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("continue")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("break")),
    Token(Token::Kind::KEYWORD_GROUP_1, macro_static_str("do")),

    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("internal")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("global")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("noexcept")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("dirty")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("sizeof")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("cast")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("constexpr")),
    Token(Token::Kind::KEYWORD_GROUP_2, macro_static_str("inline")),

    Token(Token::Kind::BUILTIN, macro_static_str("u8")),
    Token(Token::Kind::BUILTIN, macro_static_str("i8")),
    Token(Token::Kind::BUILTIN, macro_static_str("u16")),
    Token(Token::Kind::BUILTIN, macro_static_str("i16")),
    Token(Token::Kind::BUILTIN, macro_static_str("u32")),
    Token(Token::Kind::BUILTIN, macro_static_str("i32")),
    Token(Token::Kind::BUILTIN, macro_static_str("u64")),
    Token(Token::Kind::BUILTIN, macro_static_str("i64")),
    Token(Token::Kind::BUILTIN, macro_static_str("u128")),
    Token(Token::Kind::BUILTIN, macro_static_str("i128")),
    Token(Token::Kind::BUILTIN, macro_static_str("usz")),
    Token(Token::Kind::BUILTIN, macro_static_str("isz")),

    Token(Token::Kind::BUILTIN, macro_static_str("f32")),
    Token(Token::Kind::BUILTIN, macro_static_str("f64")),

    Token(Token::Kind::BUILTIN, macro_static_str("bool")),

    Token(Token::Kind::BUILTIN, macro_static_str("mc")),
    Token(Token::Kind::BUILTIN, macro_static_str("bb")),
    Token(Token::Kind::BUILTIN, macro_static_str("error")),
    Token(Token::Kind::BUILTIN, macro_static_str("str")),
    Token(Token::Kind::BUILTIN, macro_static_str("cstr")),

    Token(Token::Kind::BUILTIN, macro_static_str("nil")),
    Token(Token::Kind::BUILTIN, macro_static_str("void")),
    Token(Token::Kind::BUILTIN, macro_static_str("true")),
    Token(Token::Kind::BUILTIN, macro_static_str("false")),

    Token(Token::Kind::BUILTIN, macro_static_str("must")),
    Token(Token::Kind::BUILTIN, macro_static_str("unreachable")),
    Token(Token::Kind::BUILTIN, macro_static_str("panic")),
    Token(Token::Kind::BUILTIN, macro_static_str("nop_use")),
};

#define FLAT_MAP_CAP 256
#define FLAT_MAP_HASH_MASK 0xFF
#define FLAT_MAP_SEED 2664

// Hash table of static size with no collisions. This is achived
// by handpicking starting seed for hash function
struct FlatMap {
  // Item stored in map. Returned when get method is used
  struct Item {
    // value that was placed in map by add method
    Token::Kind value;

    // true if item is actually stored in map, i.e.
    // it is map item occupation flag
    bool ok;

    // constructs empty item
    let Item() noexcept : value(Token::Kind::EMPTY), ok(false) {}

    let Item(Token::Kind v) noexcept : value(v), ok(true) {}
  };

  // Internal structure used for storing items in map
  struct Entry {
    // key hash of stored item
    u64 hash;

    // key length of stored item
    usz len;

    Item item;
  };

  u64 seed;

  // continuous array of stored entries
  Entry* ptr;

  // number of elements stored in map
  usz len;

  // minimal length of key stored in map
  usz min_key_len;

  // maximum length of key stored in map
  usz max_key_len;

  let FlatMap(u64 s, Entry* p) noexcept
      : seed(s), ptr(p), len(0), min_key_len(0), max_key_len(0) {
    clear();
  }

  method u64 hash(mc key) noexcept { return hash::map::compute(seed, key); }

  method usz determine_pos(u64 h) noexcept { return h & FLAT_MAP_HASH_MASK; }

  // Returns true if item was successfully added to map.
  // Returns false if corresponding cell was already
  // occupied in map
  method bool add(mc key, Token::Kind value) noexcept {
    const usz h = hash(key);
    const usz pos = determine_pos(h);

    const Entry entry = ptr[pos];

    if (entry.item.ok) {
      return false;
    }

    ptr[pos] = {
        .hash = h,
        .len = key.len,
        .item = Item(value),
    };

    if (key.len > max_key_len) {
      max_key_len = key.len;
    }

    if (len == 0) {
      min_key_len = key.len;
    } else if (key.len < min_key_len) {
      min_key_len = key.len;
    }

    len += 1;

    return true;
  }

  method Item get(mc key) noexcept {
    if ((key.len < min_key_len) || (key.len > max_key_len)) {
      return Item();
    }

    const usz h = hash(key);
    const usz pos = determine_pos(h);
    const Entry entry = ptr[pos];

    if (!entry.item.ok || entry.len != key.len || entry.hash != h) {
      return Item();
    }

    return entry.item;
  }

  method void clear() noexcept {
    mc(cast(u8*, ptr), chunk_size(Entry, FLAT_MAP_CAP)).clear();
  }

  // print empty and occupied elements to buffer
  // in illustrative way
  method mc visualize(mc c) noexcept {
    var bb buf = bb(c);

    const usz row_len = 64;

    var Entry* p = ptr;
    for (usz i = 0; i < FLAT_MAP_CAP / row_len; i += 1) {
      for (usz j = 0; j < row_len; j += 1) {
        const Entry entry = p[j];

        if (entry.item.ok) {
          buf.write('X');
        } else {
          buf.write('_');
        }
      }

      p += row_len;
      buf.lf();
      buf.lf();
    }

    return buf.head();
  }
};

struct TextLine {
  // bytes that constitutes a line of text, not including
  // trailing newline characters
  mc data;
};

#include "lexer.cpp"

fn internal DynBuffer<Token> tokenize_line(FlatMap* map, mc line) noexcept {
  if (line.is_nil()) {
    return DynBuffer<Token>();
  }

  var DynBuffer<Token> buf = DynBuffer<Token>();
  var nord::Lexer lexer = nord::Lexer(map, line);

  var Token tok = lexer.lex();
  while (tok.kind != Token::Kind::EOL) {
    buf.append(tok);
  }

  return buf;
}

fn internal DynBuffer<TextLine> split_lines(mc text) noexcept {
  const usz avg_bytes_per_line = 25;  // empirical constant

  var DynBuffer<TextLine> buf =
      DynBuffer<TextLine>(text.len / avg_bytes_per_line);

  // start index of current line in text chunk
  var usz start = 0;

  // end index of current line in text chunk
  var usz end = 0;

  // line append index
  var usz j = 0;

  // text scan index
  var usz i = 0;
  while (i < text.len) {
    var u8 c = text.ptr[i];

    if (c != '\r' && c != '\n') {
      end += 1;
    }

    if (c == '\n') {
      const mc line = text.slice(start, end);
      const TextLine text_line = {.data = line};

      buf.append(text_line);

      j += 1;

      start = i + 1;
      end = start;
    }

    i += 1;
  }

  if (start == end) {
    // no need to save last line
    return buf;
  }

  // save last line
  const mc line = text.slice(start, end);
  const TextLine text_line = {.data = line};
  buf.append(text_line);

  j += 1;

  return buf;
}

fn internal mc format_gutter(bb buf, usz width, u32 line_number) noexcept {
  var usz n = buf.fmt_dec(line_number);

  for (usz i = 0; i < width - n - 3; i += 1) {
    buf.write(' ');
  }

  buf.write(' ');
  buf.write('|');
  buf.write(' ');

  return buf.head();
}

struct Editor {
  // type of input sequence
  enum struct Seq : u8 {
    // malformed sequence
    UNKNOWN = 0,

    // key is a regular printable character
    REGULAR,

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

  TerminalOutputBuffer term_buf;

  DynBuffer<TextLine> lines;

  // cursor position, originating from top-left corner
  u32 cx;
  u32 cy;

  // terminal screen sizes
  u32 rows_num;
  u32 cols_num;

  // position of text viewport top-left corner
  u32 vx;
  u32 vy;

  // number of rows in jump when page up/down is pressed
  u32 viewport_page_stride;

  bool full_viewport_upd_flag;

  let Editor() noexcept {}

  method void init() noexcept {
    init_terminal();

    term_buf.hide_cursor();
    draw_text();
    term_buf.show_cursor();
    update_cursor_position();
    term_buf.flush();
  }

  method void init(cstr filename) noexcept {
    var FileReadResult result = read_file(filename);
    if (result.is_err()) {
      // TODO: display error message
      return;
    }

    var mc text = result.data;

    lines = split_lines(text);

    init_terminal();

    term_buf.hide_cursor();
    clear_window();
    draw_text();
    term_buf.show_cursor();
    update_cursor_position();
    term_buf.flush();
  }

  method void init_terminal() noexcept {
    cx = 0;
    cy = 0;

    vx = 0;
    vy = 0;

    full_viewport_upd_flag = false;

    enter_raw_mode();

    term_buf = TerminalOutputBuffer(COMMAND_BUFFER_INITIAL_SIZE);

    struct winsize ws = get_viewport_size();
    rows_num = cast(u32, ws.ws_row);
    cols_num = cast(u32, ws.ws_col);

    viewport_page_stride = (2 * rows_num) / 3;
  }

  method void draw_text() noexcept {
    var dirty u8 gutter_buf[16];
    var bb buf = bb(gutter_buf, 16);

    var u32 max_line_number = min(vy + rows_num, cast(u32, lines.len()));
    var usz line_number_width = buf.fmt_dec(max_line_number);
    buf.reset();

    // gutter has format "xxxx | "
    // min width for line number is 4
    var usz gutter_width = max(cast(usz, 4 + 3), line_number_width + 3);

    // y coordinate inside viewport
    u32 y = 0;

    // line index which is drawn at current y coordinate
    usz j = vy;
    while (y < rows_num - 1 && j < lines.len()) {
      mc line_gutter = format_gutter(buf, gutter_width, cast(u32, j + 1));
      term_buf.write(line_gutter);
      term_buf.write(
          lines.buf.ptr[j].data.crop(cols_num - cast(u32, gutter_width)));
      term_buf.nl();

      y += 1;
      j += 1;
    }

    // draw last line without newline at the end
    if (y == rows_num - 1 && j < lines.len()) {
      mc line_gutter = format_gutter(buf, gutter_width, cast(u32, j + 1));
      term_buf.write(line_gutter);
      term_buf.write(
          lines.buf.ptr[j].data.crop(cols_num - cast(u32, gutter_width)));
    }
  }

  method void move_viewport_up() noexcept {
    if (vy == 0) {
      return;
    }
    vy -= 1;
    full_viewport_upd_flag = true;
  }

  method void move_viewport_down() noexcept {
    if (vy >= lines.len() - 1) {
      return;
    }
    vy += 1;
    full_viewport_upd_flag = true;
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
      move_viewport_up();
      return;
    }
    cy -= 1;
  }

  method void move_cursor_down() noexcept {
    if (cy >= rows_num - 1) {
      move_viewport_down();
      return;
    }
    cy += 1;
  }

  method void move_cursor_top() noexcept { cy = 0; }

  method void move_cursor_bot() noexcept { cy = rows_num - 1; }

  method void jump_viewport_up() noexcept {
    if (vy == 0) {
      return;
    }

    if (vy < viewport_page_stride) {
      vy = 0;
    } else {
      vy -= viewport_page_stride;
    }
    full_viewport_upd_flag = true;
  }

  method void jump_viewport_down() noexcept {
    if (vy >= lines.len() - 1) {
      return;
    }

    if (vy + viewport_page_stride > lines.len() - 1) {
      vy = cast(u32, lines.len()) - 1;
    } else {
      vy += viewport_page_stride;
    }
    full_viewport_upd_flag = true;
  }

  method void update_cursor_position() noexcept {
    term_buf.change_cursor_position(cx, cy);
  }

  method void update_window() noexcept {
    if (full_viewport_upd_flag) {
      clear_window();
      draw_text();
      full_viewport_upd_flag = false;
    }
    update_cursor_position();

    term_buf.flush();
  }

  method void clear_window() noexcept {
    term_buf.reset();

    term_buf.write(macro_static_str("\x1b[2J"));  // clear terminal screen
    term_buf.write(
        macro_static_str("\x1b[H"));  // position cursor at the top-left corner

    term_buf.flush();
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
      e.jump_viewport_up();
      break;
    }
    case Editor::Seq::PAGE_DOWN: {
      e.jump_viewport_down();
      break;
    }

    case Editor::Seq::DELETE: {
      break;
    }

    case Editor::Seq::UNKNOWN: {
      break;
    }
    case Editor::Seq::ESC: {
      break;
    }
    case Editor::Seq::REGULAR: {
      unreachable();
    }
    default: {
      break;
    }
  }

  e.update_window();
}

fn i32 main(i32 argc, u8** argv) noexcept {
  var FlatMap::Entry a[FLAT_MAP_CAP] dirty;

  for (u64 j = 0; j < 100000; j += 1) {
    var FlatMap m = FlatMap(j, a);
    var bool found = true;

    for (usz i = 0; i < sizeof(static_literals_table) / sizeof(Token); i += 1) {
      var Token tok = static_literals_table[i];

      var bool ok = m.add(tok.lit, tok.kind);

      if (!ok) {
        found = false;
        break;
      }
    }

    if (found) {
      var dirty u8 z[10];
      var bb b = bb(z, sizeof(z));
      b.fmt_dec(m.seed);
      stderr_write(b.head());
      stderr_write(macro_static_str("\n"));

      var dirty u8 buf[1024];
      stderr_write_all(m.visualize(mc(buf, sizeof(buf))));

      stderr_write(macro_static_str("success\n"));

      const FlatMap::Item item = m.get(macro_static_str("fn"));
      b.reset();
      b.fmt_dec(cast(u8, item.value));
      b.lf();

      stderr_write(b.head());

      exit(0);
    }
  }
  stderr_write(macro_static_str("failure\n"));
  return 1;

  if (argc < 2) {
    e.init();
  } else {
    cstr filename = cstr(argv[1]);
    e.init(filename);
  }

  while (true) {
    Editor::Key k = read_key_input();
    handle_key_input(k);
  }

  unreachable();
}
