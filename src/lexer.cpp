namespace nord {

// Lexer scans text in line and outputs tokens in sequential
// manner
struct Lexer {
  // text that is being scanned
  str text;

  // for detecting tokens with static literals
  FlatMap* map;

  // scan position
  u32 pos;

  // byte read index
  u32 i;

  // mark position
  u32 mark;

  // byte at current position
  u8 c;

  // next byte
  u8 next;

  // lexer reached end of input
  bool eof;

  let Lexer(FlatMap* m, str s) noexcept
      : text(s), map(m), pos(0), i(0), mark(0), c(0), next(0), eof(false) {
    //
    // prefill next and current bytes
    advance();
    advance();

    pos = 0;
  }

  method void advance() noexcept {
    if (eof) {
      return;
    }

    c = next;
    pos += 1;

    if (i >= text.len) {
      eof = pos >= text.len;
      return;
    }

    next = text.ptr[i];
    i += 1;
  }

  method Token lex() noexcept {
    if (eof) {
      return Token(Token::Kind::EOL);
    }

    if (text::is_simple_whitespace(c)) {
      return whitespace();
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
      return character();
    }

    if (c == '#') {
      return directive();
    }

    if (c == '/' && next == '/') {
      return comment();
    }

    return other();
  }

  method Token whitespace() noexcept {
    switch (c) {
      case ' ': {
        return space();
      }
      case '\t': {
        return tab();
      }
      case '\n': {
        return new_line();
      }
      case '\r': {
        return no_print();
      }
      default: {
        unreachable();
      }
    }
  }

  method Token space() noexcept {
    advance();
    var u16 count = 1;

    while (!eof && c == ' ') {
      advance();
      count += 1;
    }

    return Token(Token::Kind::SPACE, count);
  }

  method Token tab() noexcept {
    advance();
    var u16 count = 1;

    while (!eof && c == '\t') {
      advance();
      count += 1;
    }

    return Token(Token::Kind::TAB, count);
  }

  method Token no_print() noexcept {
    const u16 val = c;
    advance();

    return Token(Token::Kind::NO_PRINT, val);
  }

  method Token new_line() noexcept {
    advance();

    return Token(Token::Kind::NEW_LINE);
  }

  method Token word() noexcept {
    start();

    advance();

    while (!eof && text::is_alphanum(c)) {
      advance();
    }

    return Token(Token::Kind::IDENTIFIER, stop());
  }

  method Token number() noexcept {
    start();

    advance();

    while (!eof && text::is_hexadecimal_digit(c)) {
      advance();
    }

    return Token(Token::Kind::NUMBER, stop());
  }

  method Token directive() noexcept {
    start();

    advance();  // consume '#'

    while (!eof && text::is_latin_letter(c)) {
      advance();
    }

    return Token(Token::Kind::DIRECTIVE, stop());
  }

  method Token comment() noexcept {
    start();

    advance();  // consume '/'
    advance();  // consume '/'

    while (!eof) {
      advance();
    }

    return Token(Token::Kind::COMMENT, stop());
  }

  method Token string() noexcept {
    start();

    advance();  // consume '"'

    while (!eof && c != '"') {
      advance();
    }

    if (c == '"') {
      advance();
    }

    return Token(Token::Kind::STRING, stop());
  }

  method Token character() noexcept {
    start();

    advance();  // consume '\''

    while (!eof && c != '\'') {
      advance();
    }

    if (c == '\'') {
      advance();
    }

    return Token(Token::Kind::CHARACTER, stop());
  }

  method Token other() noexcept {
    advance();

    return Token(Token::Kind::PUNCTUATOR);
  }

  // place mark at current scan position
  method void start() noexcept { mark = pos; }

  // return a string with start at mark position and end at current
  // scan position
  method str stop() noexcept { return text.slice(mark, pos); }
};

}  // namespace nord

var mc token_mnemonics[] = {
    mc(macro_static_str("EMPTY")),       // EMPTY
    mc(macro_static_str("EOF")),         // EOL
    mc(macro_static_str("DIRECTIVE")),   // DIRECTIVE
    mc(macro_static_str("KEYWORD_1")),   // KEYWORD_GROUP_1
    mc(macro_static_str("KEYWORD_2")),   // KEYWORD_GROUP_2
    mc(macro_static_str("BUILTIN")),     // BUILTIN
    mc(macro_static_str("IDENTIFIER")),  // IDENTIFIER
    mc(macro_static_str("STRING")),      // STRING
    mc(macro_static_str("CHARACTER")),   // CHARACTER
    mc(macro_static_str("COMMENT")),     // COMMENT
    mc(macro_static_str("NUMBER")),      // NUMBER
    mc(macro_static_str("OPERATOR")),    // OPERATOR
    mc(macro_static_str("PUNCTUATOR")),  // PUNCTUATOR
    mc(macro_static_str("SPACE")),       // SPACE
    mc(macro_static_str("TAB")),         // TAB
    mc(macro_static_str("NEW_LINE")),    // NEW_LINE
    mc(macro_static_str("NO_PRINT")),    // NO_PRINT
};

method usz Token::fmt(mc c) noexcept {
  var bb buf = bb(c);

  const str mnemonic = token_mnemonics[cast(u8, kind)];
  buf.write(mnemonic);
  if (has_no_lit()) {
    return buf.len;
  }

  const usz mnemonic_pad_length = 16;
  buf.write_repeat(mnemonic_pad_length - mnemonic.len, ' ');

  switch (kind) {
    case Kind::EMPTY: {
      unreachable();
    }
    case Kind::EOL: {
      unreachable();
    }

    case Kind::KEYWORD_GROUP_1:
    case Kind::KEYWORD_GROUP_2:
    case Kind::DIRECTIVE:
    case Kind::BUILTIN:
    case Kind::STRING:
    case Kind::CHARACTER:
    case Kind::COMMENT:
    case Kind::NUMBER:
    case Kind::OPERATOR:
    case Kind::PUNCTUATOR:
    case Kind::IDENTIFIER: {
      buf.write(lit);
      return buf.len;
    }

    case Kind::NO_PRINT:
    case Kind::SPACE:
    case Kind::TAB: {
      buf.fmt_dec(val);
      return buf.len;
    }

    case Kind::NEW_LINE: {
      unreachable();
    }
    default: {
      unreachable();
    }
  }
}

namespace nord {

fn fs::WriteResult dump_tokens(fs::FileDescriptor fd, Lexer& lx) noexcept {
  var u8 write_buf[1 << 13];
  var fs::BufFileWriter w =
      fs::BufFileWriter(fd, mc(write_buf, sizeof(write_buf)));

  var u8 b[64] dirty;
  var mc buf = mc(b, sizeof(b));
  var Token tok dirty;
  do {
    tok = lx.lex();

    // keep 1 byte in order to guarantee enough space for
    // line feed character at the end
    const usz n = tok.fmt(buf.slice_down(1));
    buf.slice_from(n).unsafe_write('\n');

    var fs::WriteResult r = w.write(buf.slice_to(n + 1));
    if (r.is_err()) {
      return r;
    }
  } while (tok.kind != Token::Kind::EOL);

  return w.flush();
}

}  // namespace nord
