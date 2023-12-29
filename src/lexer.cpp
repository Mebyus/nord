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

  // lexer reached end of line
  bool eol;

  let Lexer(FlatMap* m, str s) noexcept
      : text(s), map(m), pos(0), i(0), mark(0), c(0), next(0), eol(false) {
    //
    // prefill next and current bytes
    advance();
    advance();

    pos = 0;
  }

  method void advance() noexcept {
    if (eol) {
      return;
    }

    c = next;
    pos += 1;

    if (i >= text.len) {
      eol = true;
      return;
    }

    next = text.ptr[i];
    i += 1;
  }

  method Token lex() noexcept {
    if (eol) {
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
        break;
      }
      case '\t': {
        break;
      }
      case '\r': {
        break;
      }
      default:
        unreachable();
    }
  }

  method Token word() noexcept {
    start();

    advance();

    while (!eol && text::is_alphanum(c)) {
      advance();
    }

    return Token(Token::Kind::IDENTIFIER, stop());
  }

  method Token number() noexcept {
    start();

    advance();

    while (!eol && text::is_hexadecimal_digit(c)) {
      advance();
    }

    return Token(Token::Kind::NUMBER, stop());
  }

  method Token directive() noexcept {
    start();

    advance();  // consume '#'

    while (!eol && text::is_latin_letter(c)) {
      advance();
    }

    return Token(Token::Kind::DIRECTIVE, stop());
  }

  method Token comment() noexcept {
    start();

    advance();  // consume '/'
    advance();  // consume '/'

    while (!eol) {
      advance();
    }

    return Token(Token::Kind::COMMENT, stop());
  }

  method Token string() noexcept {
    start();

    advance();  // consume '"'

    while (!eol && c != '"') {
      advance();
    }

    if (c == '"') {
      advance();
    }

    return Token(Token::Kind::STRING, stop());
  }

  method Token character() noexcept {}

  method Token other() noexcept {}

  // place mark at current scan position
  method void start() noexcept { mark = pos; }

  // return a string with start at mark position and end at current
  // scan position
  method str stop() noexcept { return text.slice(mark, pos); }
};

}  // namespace nord
