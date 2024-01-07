#include "core.hpp"

#include "core_linux.cpp"

namespace mimic {

// Refers to position of something in source code input stream
struct Pos {
  // Line number. Zero-based value
  //
  // To clarify: zero value means line number 1
  u32 line;

  // Column number. Zero-based value
  //
  // To clarify: zero value means column number 1
  u32 col;

  let Pos() noexcept : line(0), col(0) {}

  let Pos(u32 l, u32 c) noexcept : line(l), col(c) {}

  // Increase position to a new line. Column will be reset to 0
  method void nl() noexcept {
    line += 1;
    col = 0;
  }

  // Increase position to a new column. Line will be unchanged
  method void nc() noexcept { col += 1; }

  // Formats position into supplied memory chunk with no safety
  // checks. Returns number of bytes written
  //
  // Formatting is done in the form <line>:<column>. Both line and
  // column are represented as one-based decimal numbers. Starting
  // position is formatted as 1:1
  method usz unsafe_fmt(mc c) noexcept {
    var bb buf = bb(c);

    var usz n = buf.unsafe_fmt_dec(line);

    buf.unsafe_write(':');
    n += 1;

    n += buf.unsafe_fmt_dec(col);
    return n;
  }
};

struct Token {
  enum struct Kind : u8 {
    // Default unset value, mostly for detecting misusage
    Empty = 0,

    // Illegal byte sequences, malformed numbers, unknown directives
    Illegal,

    // End of input stream
    EOF,

    // Special C preprocessor keywords which start from # character
    Directive,

    Keyword,

    // Builtin identifiers
    Builtin,

    Identifier,

    // String literal
    String,

    // Character literal
    Charlit,

    // Integer number literal
    Integer,

    // Floating point number literal
    Float,

    Assign,

    Bracket,

    Operator,

    Punctuator,
  };

  enum struct Illegal : u8 {
    Empty = 0,

    MalformedString,

    UnrecognizedDirective,

    // Token exceeds max token length
    DirectiveOverflow,

    WordOverflow,

    StringOverflow,
  };

  enum struct Directive : u8 {
    Empty = 0,

    Include,

    Define,
    Undef,

    If,
    Elif,
    Else,
    Ifdef,
    Ifndef,
    Endif,

    Error,
  };

  enum struct Keyword : u8 {
    Empty = 0,

    Var,
    Const,
    Struct,
    Enum,
    Fn,
    Method,

    // Marks constructor
    Let,

    // Marks destructor
    Des,

    For,
    While,
    Switch,
    If,
    Else,
    Do,

    Return,
    Case,
    Default,
    Continue,
    Break,

    Typedef,
    Namespace,
    Template,
    Typename,

    Internal,
    Global,
    Dirty,
    Constexpr,
    Inline,
    Never,
  };

  enum struct Builtin : u8 {
    Empty = 0,

    Sizeof,
    Cast,

    U8,
    I8,
    U16,
    I16,
    U32,
    I32,
    U64,
    I64,
    U128,
    I128,

    Usz,
    Isz,

    F32,
    F64,
    F128,

    Bool,
    Rune,

    MC,
    BB,
    Str,
    Cstr,
    Chunk,
    Buffer,
    Error,

    Nil,
    Void,
    True,
    False,

    Must,
    Unreachable,
    Panic,
    NopUse,
  };

  enum struct Assign : u8 {
    Empty = 0,

    Regular,

    Add,
    Sub,
    Mult,
    Div,
    Rem,
  };

  enum struct Bracket : u8 {
    Empty = 0,

    LParen,
    RParen,

    LCurly,
    RCurly,

    LSquare,
    RSquare,

    LAngle,
    RAngle,
  };

  enum struct Operator : u8 {
    Empty = 0,

    Asterisk,
    Ampersand,

    Plus,
    Minus,
    Slash,
    Percent,

    Pipe,
    Caret,

    LShift,
    RShift,

    LogicalAnd,
    LogicalOr,
  };

  enum struct Punctuator : u8 {
    Empty = 0,

    Semicolon,
    Comma,
    Colon,

    Period,
    DoubleColon,

    RightArrow,
  };

  union Literal {
    // Used for token kinds from list below:
    //  - Identifier
    //  - String
    //  - Integer
    mc text;

    // If token literal fits into 23 bytes it will be
    // placed into this byte array. Last byte in this
    // array (at index 23) stores literal size in
    // bytes
    u8 s23[24];

    // Used for token kinds from list below:
    //  - Illegal:     error code
    //  - Directive:   subkind
    //  - Keyword:     subkind
    //  - Builtin:     subkind
    //  - Integer:     value (if it fits into u64)
    //  - Charlit:     character rune
    //  - Assign:      subkind
    //  - Bracket:     subkind
    //  - Operator:    subkind
    //  - Punctuator:  subkind
    //
    // Stores respective enum value of subkind
    u64 val;

    // Three u64 integers for fast s23 member comparison
    // between each other
    u64 fc[3];

    let Literal() noexcept {}

    let Literal(Illegal subkind) noexcept : val(cast(u64, subkind)) {}
  };

  enum struct Flags : u8 {
    // This flag is raised if token literal is not
    // small enough to fit into lit.s23 internal bytes array
    // and instead is stored as a string allocated somewhere
    // else
    TextLiteral = 1,
  };

  Literal lit;

  Pos pos;

  Kind kind;

  // Meaning depends on the value of field kind
  u8 flags;

  // let Token() noexcept : lit(Literal()), pos(Pos()), kind(Kind::Empty),
  // flags(0) {}

  let Token(Pos p, Kind k) noexcept
      : lit(Literal()), pos(p), kind(k), flags(0) {}

  let Token(Pos p, Illegal subkind) noexcept
      : lit(Literal(subkind)), pos(p), kind(Kind::Illegal), flags(0) {}
};

internal const usz max_token_byte_length = 1 << 10;

internal const usz max_small_token_byte_length = 23;

// Lexer scans input text in line outputs tokens in sequential
// manner
//
// Clients should only use lex method for accessing next token
struct Lexer {
  // text that is being scanned
  str text;

  // current scan position (line + column) in text
  Pos pos;

  // for detecting word tokens with special meaning:
  //  - directives
  //  - keywords
  //  - builtins
  // FlatMap* map;

  // Memory arena that is used to allocate space for
  // token literals
  mem::Arena* arena;

  // next byte read index
  u32 i;

  // current byte scan index
  u32 s;

  // Mark index
  //
  // Mark is used to slice text for token literals
  u32 mark;

  // Byte at current scan position
  //
  // This is a cached value. Look into advance() method
  // for details about how this caching algorithm works
  u8 c;

  // Next byte that will be placed at scan position
  //
  // This is a cached value from previous read
  u8 next;

  // lexer reached end of input
  bool eof;

  method void init() noexcept {
    // prefill next and current bytes
    advance();
    advance();

    // reset current scan position to start
    pos = Pos();
    s = 0;

    eof = text.len == 0;
  }

  // Advance lexer scan position one byte forward until end of
  // input is reached
  method void advance() noexcept {
    if (eof) {
      return;
    }

    if (c == '\n') {
      pos.nl();
    } else {
      pos.nc();
    }

    c = next;
    s += 1;

    if (i >= text.len) {
      eof = s >= text.len;
      return;
    }

    next = text.ptr[i];
    i += 1;
  }

  method void consume_word() noexcept {
    while (!eof && text::is_alphanum(c)) {
      advance();
    }
  }

  // create a simple token at current scan position
  method Token create(Token::Kind kind) noexcept { return Token(pos, kind); }

  method Token create_text_token(Pos p, Token::Kind kind, str ss) noexcept {
    must(!ss.is_nil());

    var Token::Literal lit = Token::Literal();
    var Token tok = Token(p, kind);

    if (ss.len > max_small_token_byte_length) {
      lit.text = arena->allocate_copy(ss);
      tok.lit = lit;
      tok.flags = cast(u8, Token::Flags::TextLiteral);
      return tok;
    }

    lit.s23[23] = cast(u8, ss.len);
    copy(ss.ptr, lit.s23, ss.len);
    tok.lit = lit;
    return tok;
  }

  method Token identifier(Pos p, str ss) noexcept {
    return create_text_token(p, Token::Kind::Identifier, ss);
  }

  // place mark at current scan position
  method void start() noexcept { mark = s; }

  // return a string with start at mark index and end at current
  // scan index
  method str stop() noexcept { return text.slice(mark, s); }

  method void skip_whitespace() noexcept {
    while (!eof && text::is_simple_whitespace(c)) {
      advance();
    }
  }

  method void skip_line() noexcept {
    while (!eof && c != '\n') {
      advance();
    }
    if (!eof) {
      // skip newline character byte at the end of line
      advance();
    }
  }

  method void skip_line_comment() noexcept {
    advance();  // skip '/'
    advance();  // skip '/'
    skip_line();
  }

  method void skip_whitespace_and_comments() noexcept {
    while (!eof) {
      skip_whitespace();
      if (c == '/' && next == '/') {
        skip_line_comment();
      } else {
        return;
      }
    }
  }

  method Token lex() noexcept {
    if (eof) {
      return create(Token::Kind::EOF);
    }

    skip_whitespace_and_comments();
    if (eof) {
      return create(Token::Kind::EOF);
    }

    if (text::is_latin_letter_or_underscore(c)) {
      return word();
    }

    if (text::is_decimal_digit(c)) {
      return number();
    }

    if (c == '"') {
      return string();
    }

    if (c == '\'') {
      return charlit();
    }

    if (c == '#') {
      return directive();
    }

    return other();
  }

  method Token word() noexcept {
    const Pos p = pos;
    start();
    advance();  // skip first symbol
    consume_word();
    var str w = stop();

    if (w.len > max_token_byte_length) {
      return Token(p, Token::Illegal::WordOverflow);
    }

    // TODO: add special tokens detection
    // const FlatMap::Item item = map->get(w);
    // if (item.ok) {
    //   return Token(item.value, w);
    // }

    return identifier(p, w);
  }

  method Token string() noexcept {
    const Pos p = pos;
    advance();  // skip '"'
    start();

    while (!eof && c != '\n' && c != '"') {
      if (c == '\\' && next == '"') {
        // skip escape sequence
        advance();
      }
      advance();
    }

    if (c != '"') {
      return Token(p, Token::Illegal::MalformedString);
    }

    var str ss = stop();
    advance();  // skip '"'

    if (ss.len > max_token_byte_length) {
      return Token(p, Token::Illegal::StringOverflow);
    }

    return create_text_token(p, Token::Kind::String, ss);
  }

  method Token directive() noexcept {
    const Pos p = pos;
    start();
    advance();  // skip '#'
    consume_word();
    var str w = stop();

    if (w.len > max_token_byte_length) {
      return Token(p, Token::Illegal::DirectiveOverflow);
    }

    // TODO: directive detection
    return Token(p, Token::Illegal::UnrecognizedDirective);
  }

  method Token other() noexcept {
    switch (c) {
      case '{': {
        break;
      }

      default: {
        break;
      }
    }
  }
};

fn void lex_file(cstr filename) noexcept {}

}  // namespace mimic

fn i32 main(i32 argc, u8** argv) noexcept {
  if (argc < 2) {
    return 1;
  }

  var cstr filename = cstr(argv[1]);
  mimic::lex_file(filename);
  return 0;
}
