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
#include <time.h>
#include <unistd.h>

#include "core.hpp"

#include "core_linux.cpp"

var coven::log::Logger lg;

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

fn internal void flush_logger() noexcept {
  lg.info(macro_static_str("nord exit"));
  lg.flush();
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
  atexit(flush_logger);
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

  let Color() noexcept : red(0), green(0), blue(0) {}
  let Color(u8 r, u8 g, u8 b) noexcept : red(r), green(g), blue(b) {}
};

constexpr bool operator==(const Color& a, const Color& b) noexcept {
  return a.red == b.red && a.green == b.green && a.blue == b.blue;
}

internal const usz terminal_sketch_buffer_size = 32;

struct TerminalOutputBuffer {
  u8 sketch_buf[terminal_sketch_buffer_size];

  DynBytesBuffer db;

  let TerminalOutputBuffer() noexcept {}

  let TerminalOutputBuffer(usz initial_size) noexcept {
    db = DynBytesBuffer(initial_size);
  }

  method void write(mc c) noexcept { db.write(c); }

  method void write_repeat(usz n, u8 x) noexcept { db.write_repeat(n, x); }

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

  method void clear_line_at_cursor() noexcept {
    write(macro_static_str("\x1b[2K"));
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

  method void enter_alt_screen() noexcept {
    write(macro_static_str("\x1b[?1049h"));
  }

  method void exit_alt_screen() noexcept {
    write(macro_static_str("\x1b[?1049l"));
  }

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
    EOF,
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
    PUNCTUATOR,
    SPACE,
    TAB,
    NEW_LINE,
    NO_PRINT,
  };

  str lit;

  u16 val;

  Token::Kind kind;

  // true for Tokens for which visual representation is not
  // the same as their byte sequence in literal
  bool is_indirect;

  let Token() noexcept
      : lit(mc()), val(0), kind(Kind::EMPTY), is_indirect(false) {}

  let Token(Token::Kind k) noexcept
      : lit(mc()), val(0), kind(k), is_indirect(false) {}

  let Token(Token::Kind k, str l) noexcept
      : lit(l), val(0), kind(k), is_indirect(false) {}

  let Token(Token::Kind k, u16 v) noexcept
      : lit(mc()), val(v), kind(k), is_indirect(false) {}

  // Output token into supplied memory chunk in
  // human readable format
  method usz fmt(mc c) noexcept;

  method bool has_no_lit() noexcept {
    return kind == Kind::EMPTY || kind == Kind::EOF || kind == Kind::NEW_LINE;
  }
};

internal const Color default_color = Color(0xBB, 0xB2, 0xBF);
internal const Color gutter_color = Color(0x64, 0x55, 0x4E);

internal const Color style[] = {
    default_color,            // EMPTY
    default_color,            // EOF
    Color(0x56, 0xB6, 0xC2),  // DIRECTIVE
    Color(0xE0, 0x6C, 0x75),  // KEYWORD_GROUP_1
    Color(0xE4, 0x8A, 0x38),  // KEYWORD_GROUP_2
    Color(0x61, 0xAF, 0xEF),  // BUILTIN
    default_color,            // IDENTIFIER
    Color(0xE5, 0xC0, 0x7B),  // STRING
    Color(0xE5, 0xC0, 0x7B),  // CHARACTER
    Color(0x84, 0xAC, 0x6E),  // COMMENT
    Color(0xC6, 0x78, 0xDD),  // NUMBER
    default_color,            // OPERATOR
    default_color,            // PUNCTUATOR
    default_color,            // SPACE
    default_color,            // TAB
    default_color,            // NEW_LINE
    default_color,            // NO_PRINT
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

  // continuous chunk of stored entries
  chunk<Entry> entries;

  u64 mask;

  u64 seed;

  // number of elements stored in map
  usz len;

  // minimal length of key stored in map
  usz min_key_len;

  // maximum length of key stored in map
  usz max_key_len;

  let FlatMap() noexcept
      : entries(chunk<Entry>()),
        mask(0),
        seed(0),
        len(0),
        min_key_len(0),
        max_key_len(0) {}

  let FlatMap(usz cap, u64 m, u64 s) noexcept
      : entries(chunk<Entry>(nil, cap)),
        mask(m),
        seed(s),
        len(0),
        min_key_len(0),
        max_key_len(0) {
    //
    init();
  }

  method void init() noexcept {
    const usz cap = entries.len;
    entries = mem::calloc<Entry>(cap);
  }

  method bool is_nil() noexcept { return entries.is_nil(); }

  method bool is_empty() noexcept { return len == 0; }

  method void free() noexcept { mem::free(entries); }

  method u64 hash(mc key) noexcept { return hash::map::compute(seed, key); }

  method usz determine_pos(u64 h) noexcept { return h & mask; }

  // Returns true if item was successfully added to map.
  // Returns false if corresponding cell was already
  // occupied in map
  method bool add(str key, Token::Kind value) noexcept {
    const usz h = hash(key);
    const usz pos = determine_pos(h);

    const Entry entry = entries.ptr[pos];

    if (entry.item.ok) {
      return false;
    }

    entries.ptr[pos] = {
        .hash = h,
        .len = key.len,
        .item = Item(value),
    };

    if (key.len > max_key_len) {
      max_key_len = key.len;
    }

    if (is_empty()) {
      min_key_len = key.len;
    } else if (key.len < min_key_len) {
      min_key_len = key.len;
    }

    len += 1;

    return true;
  }

  method Item get(str key) noexcept {
    if ((key.len < min_key_len) || (key.len > max_key_len)) {
      return Item();
    }

    const usz h = hash(key);
    const usz pos = determine_pos(h);
    const Entry entry = entries.ptr[pos];

    if (!entry.item.ok || entry.len != key.len || entry.hash != h) {
      return Item();
    }

    return entry.item;
  }

  method void clear() noexcept {
    entries.clear();

    len = 0;
    min_key_len = 0;
    max_key_len = 0;
  }

  // print empty and occupied elements to buffer
  // in illustrative way
  method mc visualize(mc c) noexcept {
    var bb buf = bb(c);

    const usz row_len = 64;

    var Entry* p = entries.ptr;
    for (usz i = 0; i < entries.len / row_len; i += 1) {
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

var internal FlatMap token_kind_map = FlatMap(256, 0xFF, FLAT_MAP_SEED);

#include "lexer.cpp"

fn DynBuffer<Token> tokenize_line(FlatMap* map, mc line) noexcept {
  if (line.is_nil()) {
    return DynBuffer<Token>();
  }

  var DynBuffer<Token> buf = DynBuffer<Token>();
  var nord::Lexer lx = nord::Lexer(map, line);

  var Token tok = lx.lex();
  while (tok.kind != Token::Kind::EOF) {
    buf.append(tok);
    tok = lx.lex();
  }

  return buf;
}

struct TextLine {
  DynBuffer<Token> tokens;

  DynBytesBuffer changed;

  // bytes that constitutes a line of text, not including
  // trailing newline characters
  mc data;

  bool tokenized;

  let TextLine(str text) noexcept
      : tokens(DynBuffer<Token>()), data(text), tokenized(false) {}

  method str content() noexcept {
    if (!changed.is_nil()) {
      return changed.head();
    }
    return data;
  }

  method void crop(usz n) noexcept {
    tokenized = false;
    if (!changed.is_nil()) {
      changed.reset(n);
      return;
    }
    data = data.slice_to(n);
  }

  method void append(str text) noexcept {
    if (text.is_nil()) {
      return;
    }
    tokenized = false;
    if (changed.is_nil()) {
      changed = DynBytesBuffer(data);
    }
    changed.write(text);
  }

  method void insert(usz i, u8 x) noexcept {
    if (changed.is_nil()) {
      changed = DynBytesBuffer(data);
    }
    changed.insert(i, x);
    tokenize(&token_kind_map);
  }

  method void remove(usz i) noexcept {
    if (changed.is_nil()) {
      changed = DynBytesBuffer(data);
    }
    changed.remove(i);
    tokenize(&token_kind_map);
  }

  method void tokenize(FlatMap* map) noexcept {
    tokenized = true;
    tokens.reset();

    var str s = content();
    if (s.is_nil()) {
      return;
    }

    var nord::Lexer lx = nord::Lexer(map, s);
    var Token tok = lx.lex();
    while (tok.kind != Token::Kind::EOF) {
      tokens.append(tok);
      tok = lx.lex();
    }
  }

  method void free() noexcept {
    tokens.free();
    changed.free();
  }
};

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
      const TextLine text_line = TextLine(line);

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
  const TextLine text_line = TextLine(line);
  buf.append(text_line);

  j += 1;

  return buf;
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
    BACKSPACE,
    ENTER,
  };

  struct Key {
    // regular character
    u8 c;

    Seq s;
  };

  TerminalOutputBuffer term_buf;

  DynBuffer<TextLine> lines;

  // path to a file being edited
  str filename;

  // cursor position, originating from top-left corner
  u32 cx;
  u32 cy;

  // terminal screen sizes
  u32 rows_num;
  u32 cols_num;

  // number of rows and columns in text viewport
  u32 vrows;
  u32 vcols;

  // position of text viewport top-left corner
  u32 vx;
  u32 vy;

  // cursor position inside text viewport
  u32 tx;
  u32 ty;

  // number of rows in jump when page up/down is pressed
  u32 viewport_page_stride;

  // value at current viewport position
  u32 gutter_width;

  Color text_color;

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

  method void init(str name) noexcept {
    var fs::FileReadResult result = fs::read_file(name);
    if (result.is_err()) {
      // TODO: display error message
      return;
    }

    filename = name;
    var mc text = result.data;

    lines = split_lines(text);

    init_terminal();

    term_buf.hide_cursor();
    term_buf.set_background_color(Color(0x20, 0x20, 0x20));
    term_buf.flush();
    clear_window();
    draw_text();
    update_cursor_position();
    term_buf.show_cursor();
    term_buf.flush();
  }

  method void change_text_color(Color color) noexcept {
    if (text_color == color) {
      return;
    }

    term_buf.set_text_color(color);
  }

  method void init_terminal() noexcept {
    cx = 0;
    cy = 0;

    vx = 0;
    vy = 0;

    tx = 0;
    ty = 0;

    full_viewport_upd_flag = false;

    enter_raw_mode();

    term_buf = TerminalOutputBuffer(COMMAND_BUFFER_INITIAL_SIZE);
    term_buf.enter_alt_screen();
    term_buf.flush();

    struct winsize ws = get_viewport_size();
    rows_num = cast(u32, ws.ws_row);
    cols_num = cast(u32, ws.ws_col);
    vrows = rows_num - 1;
    vcols = cols_num - 6;

    viewport_page_stride = (2 * rows_num) / 3;
  }

  method void save_file() noexcept {
    lg.info(macro_static_str("saving file"));
    const fs::OpenResult r = fs::create(filename);
    if (r.is_err()) {
      lg.error(macro_static_str("failed to create file"));
      return;
    }

    var u8 write_buf[1 << 13] dirty;
    var fs::BufFileWriter w =
        fs::BufFileWriter(r.fd, mc(write_buf, sizeof(write_buf)));

    for (usz i = 0; i < lines.len(); i += 1) {
      var fs::WriteResult wr = w.write(lines.buf.ptr[i].content());
      if (wr.is_err()) {
        return;
      }

      wr = w.lf();
      if (wr.is_err()) {
        return;
      }
    }

    w.close();  // TODO: check this error
    lg.info(macro_static_str("file saved"));
  }

  method void draw_text() noexcept {
    update_gutter_width();

    // y coordinate inside viewport
    var u32 y = 0;

    const u32 max_text_width = cols_num - cast(u32, gutter_width);

    // line index which is drawn at current y coordinate
    var usz j = vy;
    while (y < vrows - 1 && j < lines.len()) {
      draw_gutter(cast(u32, j + 1));
      // change_text_color(default_color);
      draw_line(j, max_text_width);
      term_buf.nl();

      y += 1;
      j += 1;
    }

    // draw last line without newline at the end
    if (y == vrows - 1 && j < lines.len()) {
      draw_gutter(cast(u32, j + 1));
      // term_buf.set_text_color(default_color);
      draw_line(j, max_text_width);
    }
  }

  method void update_gutter_width() noexcept {
    var u8 gutter_buf[16] dirty;
    var bb buf = bb(gutter_buf, sizeof(gutter_buf));

    const u32 max_line_number = min(vy + rows_num, cast(u32, lines.len()));
    const usz line_number_width = buf.unsafe_fmt_dec(max_line_number);

    // gutter has format "xxxx  " with number aligned to right
    // min width for line number is 4
    gutter_width = cast(u32, max(cast(usz, 4 + 2), line_number_width + 2));
    vcols = cols_num - gutter_width;
  }

  method void draw_gutter(u32 line_number) noexcept {
    change_text_color(gutter_color);

    var u8 gutter_buf[16] dirty;
    var bb buf = bb(gutter_buf, sizeof(gutter_buf));

    var usz n = buf.fmt_dec(line_number);
    buf.write_repeat(gutter_width - n, ' ');
    term_buf.write(buf.head());
  }

  method void draw_line(usz k, u32 max_width) noexcept {
    nop_use(max_width);

    if (!lines.buf.ptr[k].tokenized) {
      lines.buf.ptr[k].tokenize(&token_kind_map);
    }

    var TextLine line = lines.buf.ptr[k];

    for (usz i = 0; i < line.tokens.buf.len; i += 1) {
      var Token tok = line.tokens.buf.ptr[i];

      if (tok.kind == Token::Kind::SPACE) {
        term_buf.write_repeat(cast(usz, tok.val), ' ');
      } else {
        change_text_color(style[cast(usz, tok.kind)]);
        term_buf.write(tok.lit);
      }
    }
  }

  method void redraw_line_at_cursor() noexcept {
    term_buf.hide_cursor();
    term_buf.clear_line_at_cursor();
    term_buf.change_cursor_position(0, cy);

    const u32 line_index = vy + cy;
    draw_gutter(line_index + 1);
    draw_line(line_index, 100);
  }

  method void insert_at_cursor(u8 x) noexcept {
    const usz line_index = vy + ty;
    const usz insert_index = vx + tx;
    lines.buf.ptr[line_index].insert(insert_index, x);

    // move cursor to next column after inserting a character
    tx += 1;

    redraw_line_at_cursor();
    update_cursor_position();
    term_buf.show_cursor();
    term_buf.flush();
  }

  method void delete_at_cursor() noexcept {
    const usz line_index = vy + ty;
    const usz remove_index = vx + tx;
    lines.buf.ptr[line_index].remove(remove_index);
    redraw_line_at_cursor();
    sync_cursor_position();
    term_buf.show_cursor();
    term_buf.flush();
  }

  method void backspace_at_cursor() noexcept {
    const u32 line_index = vy + ty;
    const u32 cursor_index = vx + tx;

    if (cursor_index == 0) {
      if (line_index == 0) {
        return;
      }

      const u32 prev_line_length =
          cast(u32, lines.buf.ptr[line_index - 1].content().len);
      var str text = lines.buf.ptr[line_index].content();
      lines.buf.ptr[line_index - 1].append(text);
      lines.buf.ptr[line_index].free();
      lines.remove(line_index);

      move_cursor_up();
      tx = prev_line_length;

      full_viewport_upd_flag = true;
      return;
    }

    const u32 remove_index = cursor_index - 1;
    lines.buf.ptr[line_index].remove(remove_index);

    // move cursor to previous column after backspacing a character
    tx -= 1;

    redraw_line_at_cursor();
    term_buf.show_cursor();
  }

  method void split_line_at_cursor() noexcept {
    const usz line_index = vy + ty;
    const usz split_index = vx + tx;

    lines.insert(
        line_index + 1,
        TextLine(lines.buf.ptr[line_index].content().slice_from(split_index)));

    lines.buf.ptr[line_index].crop(split_index);

    ty += 1;
    tx = 0;

    full_viewport_upd_flag = true;
  }

  method void trim_cursor_position_by_line_length() {
    tx = min(tx, current_line_length());
  }

  method u32 current_line_length() noexcept {
    const u32 line_index = vy + ty;
    return cast(u32, lines.buf.ptr[line_index].content().len);
  }

  method void move_cursor_up_line_end() noexcept {
    move_cursor_up();
    tx = current_line_length();
  }

  method void move_cursor_down_line_start() noexcept {
    move_cursor_down();
    tx = 0;
  }

  method void move_viewport_up() noexcept {
    if (vy == 0) {
      return;
    }
    vy -= 1;
    full_viewport_upd_flag = true;
  }

  method void move_viewport_down() noexcept {
    if (vy + ty >= lines.len() - 1) {
      return;
    }
    vy += 1;
    full_viewport_upd_flag = true;
  }

  method void move_cursor_right() noexcept {
    if (tx >= vcols - 1) {
      return;
    }
    if (tx >= current_line_length()) {
      move_cursor_down_line_start();
      return;
    }
    tx += 1;
  }

  method void move_cursor_left() noexcept {
    if (tx == 0) {
      move_cursor_up_line_end();
      return;
    }
    tx -= 1;
  }

  method void move_cursor_up() noexcept {
    if (ty == 0) {
      move_viewport_up();
      return;
    }
    ty -= 1;
  }

  method void move_cursor_down() noexcept {
    if (ty >= vrows - 1) {
      move_viewport_down();
      return;
    }
    ty += 1;
  }

  method void move_cursor_top() noexcept { ty = 0; }

  method void move_cursor_bot() noexcept { ty = vrows - 1; }

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
    if (vy + ty >= lines.len() - 1) {
      return;
    }

    if (vy + vrows + viewport_page_stride > lines.len() - 1) {
      vy = cast(u32, lines.len()) - vrows;
    } else {
      vy += viewport_page_stride;
    }
    full_viewport_upd_flag = true;
  }

  method void update_cursor_position() noexcept {
    trim_cursor_position_by_line_length();
    cx = gutter_width + tx;
    cy = ty;
    sync_cursor_position();
  }

  method void sync_cursor_position() noexcept {
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

  if (c != 0x1B) {
    if (c == 0x0D) {
      return Editor::Key{.c = 0, .s = Editor::Seq::ENTER};
    }
    if (c == 0x7F) {
      return Editor::Key{.c = 0, .s = Editor::Seq::BACKSPACE};
    }
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
      e.term_buf.exit_alt_screen();
      e.term_buf.flush();
      exit(0);
    }

    if (k.c == CTRL_KEY('s')) {
      e.save_file();
      return;
    }

    if (text::is_printable_ascii_character(k.c)) {
      e.insert_at_cursor(k.c);
      return;
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
      e.delete_at_cursor();
      return;
    }

    case Editor::Seq::BACKSPACE: {
      e.backspace_at_cursor();
      break;
    }

    case Editor::Seq::ENTER: {
      e.split_line_at_cursor();
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

// Adds elements to map one by one until all elements are
// added or map is unable to hold next element
//
// Returns true if all elements were successfully added
// to map. When true is returned function guarantees that
// number of supplied literals equals resulting map.len
//
// Returns false as soon as at least one element was not
// successfully added. In this case number of elements
// actually stored inside a map is unpredictable
fn bool populate_flat_map(FlatMap& map, chunk<Token> literals) noexcept {
  for (usz i = 0; i < literals.len; i += 1) {
    const Token tok = literals.ptr[i];
    const bool ok = map.add(tok.lit, tok.kind);

    if (!ok) {
      return false;
    }
  }

  return true;
}

// Allocates a map of specified (cap) size on the heap and
// picks a special seed for map hashing function which allows
// to store all strings from literals chunk in map without
// collisions
//
// Argument cap must be a power of 2. Argument mask must correspond
// to that power with respective number of 1's in lower bits.
// Number of supplied literals must be less than specified capacity.
// It is recommended to pick capacity approximately five times
// greater than number of literals
fn FlatMap fit_into_flat_map(usz cap,
                             u64 mask,
                             chunk<Token> literals) noexcept {
  //
  var FlatMap map = FlatMap(cap, mask, 0);

  for (u64 j = 0; j < 100000; j += 1) {
    const bool ok = populate_flat_map(map, literals);

    if (ok) {
      return map;
    }

    map.clear();
  }

  map.free();
  return FlatMap();
}

fn i32 main(i32 argc, u8** argv) noexcept {
  lg.init(macro_static_str("log.log"));
  lg.info(macro_static_str("nord start"));
  // lg.flush();

  populate_flat_map(token_kind_map,
                    chunk<Token>(cast(Token*, static_literals_table), 59));
  if (argc < 2) {
    e.init();
  } else {
    var cstr filename = cstr(argv[1]);
    e.init(filename.as_str());
  }

  while (true) {
    const Editor::Key k = read_key_input();
    handle_key_input(k);
  }

  unreachable();
}
