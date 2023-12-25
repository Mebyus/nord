namespace nord {

// Lexer scans text in line and outputs tokens in sequential
// manner
struct Lexer {
  mc text;

  FlatMap* map;

  u32 pos;

  // byte at current position
  u8 c;

  // next byte
  u8 next;

  // lexer reached end of line
  bool eol;

  let Lexer(FlatMap* m, mc t) noexcept
      : text(t), map(m), pos(0), c(0), next(0), eol(false) {}

  method void advance() noexcept {
    if (eol) {
      return;
    }

    c = next;

    if (pos >= text.len) {
      eol = true;
      return;
    }
    next = text.ptr[pos];
    pos += 1;
  }

  method Token lex() noexcept {
    if (eol) {
      return Token(Token::Kind::EOL);
    }
    advance();
    return Token();
  }
};

}  // namespace nord
