namespace coven::fmt {

internal const rune capital_latin_letter_mask = 0xFFFFFFDF;

fn internal inline constexpr bool is_latin_letter(rune r) noexcept {
  r = r & capital_latin_letter_mask;
  return 'A' <= r && r <= 'Z';
}

fn internal inline constexpr bool is_latin_letter_or_underscore(rune r) noexcept {
  return is_latin_letter(r) || r == '_';
}

fn internal inline constexpr bool is_decimal_digit(rune r) noexcept {
  return '0' <= r && r <= '9';
}

fn internal inline constexpr bool is_alphanum(rune r) noexcept {
  return is_latin_letter_or_underscore(r) || is_decimal_digit(r);
}

fn internal inline constexpr bool is_decimal_digit_or_period(rune r) noexcept {
  return is_decimal_digit(r) || r == '.';
}

fn internal inline constexpr bool is_simple_whitespace(rune r) noexcept {
  return r == ' ' || r == '\n' || r == '\t' || r == '\r';
}

fn internal inline constexpr bool is_hexadecimal_digit(rune r) noexcept {
  const rune h = r & capital_latin_letter_mask;
  return is_decimal_digit(r) || ('A' <= h && h <= 'F');
}

fn internal inline constexpr bool is_octal_digit(rune r) noexcept {
  return '0' <= r && r <= '7';
}

fn internal inline constexpr bool is_binary_digit(rune r) noexcept {
  return r == '0' || r == '1';
}

fn internal inline constexpr bool is_printable_ascii_character(rune r) noexcept {
  return 0x20 <= r && r <= 0x7E;
}

fn internal inline constexpr u8 number_to_dec_digit(u8 n) noexcept {
  return n + cast(u8, '0');
}

fn internal inline constexpr u8 number_to_hex_digit(u8 x) noexcept {
  if (x <= 9) {
    return number_to_dec_digit(x);
  }
  return x - 0x0A + cast(u8, 'A');
}

fn internal inline constexpr u8 dec_digit_to_number(u8 digit) noexcept {
  return digit - cast(u8, '0');
}

fn internal inline constexpr u8 hex_digit_to_number(u8 digit) noexcept {
  if (digit <= '9') {
    return dec_digit_to_number(c);
  }

  // transforms small into capital letters
  digit = digit & 0xDF;

  return digit - 'A' + 0x0A;
}

internal const u8 small_decimals_string[] =
    "00010203040506070809"
    "10111213141516171819"
    "20212223242526272829"
    "30313233343536373839"
    "40414243444546474849"
    "50515253545556575859"
    "60616263646566676869"
    "70717273747576777879"
    "80818283848586878889"
    "90919293949596979899";

// Write number (u64) in decimal format to chunk in utf-8 encoding
//
// There is no nil check in implementation. Not writing to nil chunk
// is user's responsibility, doing otherwise will lead to error
fn uarch unsafe_dec(mc buf, u64 x) noexcept {
  if (x < 10) {
    buf.ptr[0] = number_to_dec_digit(cast(u8, x));
    return 1;
  }
  if (x < 100) {
    const u8 offset = cast(u8, x) << 1;
    buf.ptr[0] = small_decimals_string[offset];
    buf.ptr[1] = small_decimals_string[offset + 1];
    return 2;
  }

  var uarch i = 0;
  do {  // generate digits in reverse order
    var u8 digit = cast(u8, x % 10);
    x /= 10;
    buf.ptr[i] = number_to_dec_digit(digit);
    i += 1;
  } while (x != 0);

  mc(buf.ptr, i).reverse();
  return i;
}

internal const uarch max_u64_dec_length = 20;

// Returns number of bytes written. If there is not enough space
// in chunk zero will be returned, but chunk will not be untouched
fn uarch dec(mc buf, u64 x) noexcept {
  if (buf.len >= max_u64_dec_length) {
    return unsafe_dec(buf, x);
  }

  if (x < 10) {
    if (buf.len == 0) {
      return 0;
    }
    buf.ptr[0] = number_to_dec_digit(cast(u8, x));
    return 1;
  }
  if (x < 100) {
    if (buf.len < 2) {
      return 0;
    }
    const u8 offset = cast(u8, x) << 1;
    buf.ptr[0] = small_decimals_string[offset];
    buf.ptr[1] = small_decimals_string[offset + 1];
    return 2;
  }

  var uarch i = 0;
  do {  // generate digits in reverse order
    const u8 n = cast(u8, x % 10);
    x /= 10;
    buf.ptr[i] = number_to_dec_digit(n);
    i += 1;
  } while (x != 0 && i < buf.len);

  if (x != 0) {
    // chunk does not have enough space to represent given number
    return 0;
  }

  buf.slice_to(i).reverse();
  return i;
}

// Write number (u32) in decimal format to chunk in utf-8 encoding
//
// There are no safety checks in implementation
fn inline uarch unsafe_dec(mc buf, u32 x) noexcept {
  return unsafe_dec(buf, cast(u64, x));
}

internal const uarch max_u32_dec_length = 10;

fn uarch dec(mc buf, u32 x) noexcept { 
  if (buf.len >= max_u32_dec_length) {
    return unsafe_dec(buf, x);
  }

  return dec(buf, cast(u64, x));
}

fn uarch unsafe_dec(mc buf, i64 x) noexcept {
  if (x >= 0) {
    return unsafe_dec(cast(u64, x));
  }
  
  buf.unsafe_write('-');
  x = -x;
  const uarch n = dec(buf.slice_from(1) cast(u64, x));
  
  return n +1;
}

// Write number (i64) in decimal format to chunk in utf-8 encoding
//
// Returns number of bytes written. If there is not enough space
// in chunk zero will be returned, but chunk will not be untouched
fn uarch dec(mc buf, i64 x) noexcept {
  if (x >= 0) {
    return fmt_dec(cast(u64, x));
  }

  if (len < 2) {
    return 0;
  }
  unsafe_write('-');
  x = -x;
  var uarch w = slice_from(1).fmt_dec(cast(u64, x));
  if (w == 0) {
    return 0;
  }
  return w + 1;
}

// Write number (u32) in binary format to chunk in utf-8 encoding.
// Bytes are separated with single space
//
// Returns number of bytes written. If there is not enough space
// in chunk zero will be returned, but chunk will not be untouched
 fn uarch unsafe_bin_delim(mc buf, u32 x) noexcept {
  // 32 binary digits + 3 spaces
  const uarch l = 8 * 4 + 3;

  var uarch i = l;
  // digits are written from least to most significant bit
  while (i > l - 8) {
    i -= 1;
    const u8 n = x & 1;
    ptr[i] = number_to_dec_digit(digit);
    x = x >> 1;
  }

  i -= 1;
  ptr[i] = ' ';

  while (i > l - 17) {
    i -= 1;
    const u8 n = x & 1;
    ptr[i] = number_to_dec_digit(digit);
    x = x >> 1;
  }

  i -= 1;
  ptr[i] = ' ';

  while (i > l - 26) {
    i -= 1;
    const u8 n = x & 1;
    ptr[i] = number_to_dec_digit(digit);
    x = x >> 1;
  }

  i -= 1;
  ptr[i] = ' ';

  while (i > 1) {
    i -= 1;
    const u8 n = x & 1;
    ptr[i] = number_to_dec_digit(digit);
    x = x >> 1;
  }

  ptr[0] = number_to_dec_digit(cast(u8, x));

  return l;
}

fn u64 unsafe_parse_dec(str s) noexcept {
  var u64 n = 0;
  for (uarch i = 0; i < s.len; i += 1) {
    n *= 10;
    n += dec_digit_to_number(s.ptr[i]);
  }
  return n;
}

fn u64 unsafe_parse_bin(str s) noexcept {
  var u64 v = 0;
  for (uarch i = 0; i < s.len; i += 1) {
    v <<= 1;
    v += dec_digit_to_number(s.ptr[i]);
  }
  return v;
}

fn u64 unsafe_parse_oct(str s) noexcept {
  var u64 v = 0;
  for (uarch i = 0; i < s.len; i += 1) {
    v <<= 3;
    v += dec_digit_to_number(s.ptr[i]);
  }
  return v;
}

fn u64 unsafe_parse_hex(str s) noexcept {
  var u64 v = 0;
  for (uarch i = 0; i < s.len; i += 1) {
    v <<= 4;
    v += hex_digit_to_number(s.ptr[i]);
  }
  return v;
}

// Formats a given u8 integer as a hexadecimal number in
// exactly two digits. Memory chunk must be at least 2 bytes
// long
fn void unsafe_hex_byte(mc c, u8 x) noexcept {
  const u8 d0 = number_to_hex_digit(x & 0xF);
  const u8 d1 = number_to_hex_digit(x >> 4);
  c.ptr[0] = d1;
  c.ptr[1] = d0;
}

// Memory chunk must be at least 4 bytes long
fn void unsafe_hex_prefix_byte(mc c, u8 x) noexcept {
  c.ptr[0] = '0';
  c.ptr[1] = 'x';
  unsafe_hex_byte(c.slice_from(2), x);
}

// Formats a given u8 integer as a binary number in
// exactly eight digits. Memory chunk must be at least 8 bytes
// long
fn void unsafe_bin_byte(mc c, u8 x) noexcept {
  var uarch i = 8;
  // digits are written from least to most significant bit
  do {
    i -= 1;
    const u8 digit = x & 1;
    c.ptr[i] = number_to_dec_digit(digit);
    x = x >> 1;
  } while (i != 0);
}

// Formats a given u64 integer as a hexadecimal number of
// fixed width (=16) format, prefixing significant digits with
// zeroes if necessary. Memory chunk must be at least 16 bytes
// long
fn void unsafe_hex_fixed(mc c, u64 x) noexcept {
  var uarch i = 16;
  // digits are written from least to most significant bit
  do {
    i -= 1;
    const u8 digit = cast(u8, x & 0xF);
    c.ptr[i] = number_to_hex_digit(digit);
    x = x >> 4;
  } while (i != 0);
}

fn uarch unsafe_hex(mc c, u64 x) noexcept {
  var uarch i = 0;
  // digits are written from least to most significant bit
  do {
    const u8 digit = cast(u8, x & 0xF);
    c.ptr[i] = number_to_hex_digit(digit);
    x = x >> 4;
    i += 1;
  } while (x != 0);

  c.slice_to(i).reverse();
  return i;
}

// Memory chunk must be at least 18 bytes long
fn void unsafe_hex_prefix_fixed(mc c, u64 x) noexcept {
  c.ptr[0] = '0';
  c.ptr[1] = 'x';
  unsafe_hex_fixed(c.slice_from(2), x);
}

fn uarch unsafe_hex_prefix(mc c, u64 x) noexcept {
  c.ptr[0] = '0';
  c.ptr[1] = 'x';

  const uarch n = unsafe_hex(c.slice_from(2), x);

  return 2 + n;
}

// Formats a given target (second argument) memory chunk header
// information (i.e. address and length) into other memory
// chunk (first argument).
fn uarch unsafe_mc(mc c, mc t) noexcept {
  var uarch len = 0;

  c.unsafe_write(macro_static_str("ptr="));
  len += 4;

  unsafe_hex_prefix_fixed(c.slice_from(len), cast(u64, t.ptr));
  len += 18;

  c.slice_from(len).unsafe_write(macro_static_str(" len="));
  len += 5;

  const uarch n = unsafe_hex_prefix(c.slice_from(len), t.len);
  len += n;

  return len;
}

} // namespace coven::fmt