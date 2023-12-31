#ifndef GUARD_CORE_HPP
#define GUARD_CORE_HPP

// Use only for comparison with pointer types
#define nil 0

#undef min
#undef max
#undef EOF

// Modify function or constant to be bound to translation unit
#define internal static

// Modify global variable to be bound to translation unit
#define global static
#define persist static

// Macro for type cast
#define cast(T, x) (T)(x)

// Mark function declaration and definition
#define fn

// Mark variable declaration
#define var

// Mark uninitialized variable
#define dirty

// Mark struct or class method
#define method

// Mark struct or class constructor
#define let

// Mark struct or class destructor
#define des ~

// Macro to compute chunk size in bytes to hold n elements of type T
#define chunk_size(T, n) ((n) * sizeof(T))

#define bit_cast(T, x) __builtin_bit_cast(T, x)

// Dummy usage for variable, argument or constant
//
// Suppresses compiler warnings
#define nop_use(x) (void)(x)

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef __uint128_t u128;

typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long long int i64;
typedef __int128_t i128;

typedef float f32;
typedef double f64;
typedef __float128 f128;

// Platform dependent unsigned integer type. Always have enough bytes to
// represent size of any memory region (or offset in memory region)
typedef u64 usz;
typedef i64 isz;

// Type for performing arbitrary arithmetic pointer operations
typedef usz uptr;

typedef void* anyptr;

// Rune represents a unicode code point
typedef u32 rune;

template <typename T>
fn inline constexpr T min(T a, T b) noexcept {
  if (a < b) {
    return a;
  }
  return b;
}

template <typename T>
fn inline constexpr T max(T a, T b) noexcept {
  if (a < b) {
    return b;
  }
  return a;
}

template <typename T>
fn inline void swap(T& a, T& b) noexcept {
  T x = a;
  a = b;
  b = x;
}

// Copies n bytes of memory from src to dst
//
// Do not use for overlapping memory regions
//
// Implementation is platform-specific and will likely wrap
// memory copy system call
fn void copy(u8* src, u8* dst, usz n) noexcept;

// Copies n bytes of memory from src to dst
//
// Guarantees correct behaviour for overlapping memory regions
fn void move(u8* src, u8* dst, usz n) noexcept;

// Small object that holds information about non-fatal error
struct error {
  // Unique identifier of error origin location in code base
  usz origin;

  //
  usz kind;

  method bool is_nil() noexcept { return kind == 0; }
};

// Memory Chunk
//
// Represents continuous region in memory in form of raw bytes.
// Chunk by definition is nil when number of bytes in it equals zero
//
// Meaning of chunk contents are dependant on its specific usage. For example
// bytes can represent a utf-8 string, bitmap of an image or a network packet.
// Users should specify meaning of chunk in place where they use it
//
// Chunk can point to static, stack or heap allocated memory. It is
// responsibility of the user to keep track of different usages and deallocate
// backing memory. Thus chunk does not own memory, it must be borrowed from
// somewhere else
struct mc {
  // Pointer to first byte in chunk
  u8* ptr;

  // Number of bytes in chunk
  usz len;

  // Create nil memory chunk
  let constexpr mc() noexcept : ptr(nil), len(0) {}

  let mc(u8* bytes, usz n) noexcept : ptr(bytes), len(n) {}

  // Fill chunk with zero bytes
  method void clear() noexcept;

  // Fill chunk with specified byte
  method void fill(u8 x) noexcept;

  // Parameters must satisfy 0 <= start <= end <= len
  //
  // Method does not perform any safety checks
  //
  // If start = end then resulting slice will be nil
  method mc slice(usz start, usz end) noexcept {
    return mc(ptr + start, end - start);
  }

  method mc slice_from(usz start) noexcept { return slice(start, len); }

  method mc slice_to(usz end) noexcept { return slice(0, end); }

  // Slice a portion of memory chunk starting from the start of
  // original slice
  //
  // In contrast with slice_to method argument specifies how many
  // bytes should be cut from the end of original slice
  method mc slice_down(usz size) noexcept { return slice_to(len - size); }

  // Return a slice from original chunk that contains at most
  // <size> bytes
  method mc crop(usz size) noexcept { return slice_to(min(len, size)); }

  method bool is_nil() noexcept { return len == 0; }

  method usz write(u8* bytes, usz n) noexcept {
    const usz w = min(n, len);
    if (w == 0) {
      return 0;
    }

    copy(bytes, ptr, w);
    return w;
  }

  // Write (copy) given chunk to this one. Writing starts from
  // first byte of both chunks
  //
  // Returns number of bytes written. Return value may be smaller than
  // expected or even 0 if memory copy system call fails
  method usz write(mc c) noexcept { return write(c.ptr, c.len); }

  // Write single byte to chunk
  method usz write(u8 x) noexcept {
    if (is_nil()) {
      return 0;
    }
    return unsafe_write(x);
  }

  method usz unsafe_write(u8* bytes, usz n) noexcept {
    copy(bytes, ptr, n);
    return n;
  }

  method usz unsafe_write(mc c) noexcept {
    copy(c.ptr, ptr, c.len);
    return c.len;
  }

  // Write single byte to chunk
  method usz unsafe_write(u8 x) noexcept {
    ptr[0] = x;
    return 1;
  }

  // Reverse bytes in chunk
  method void reverse() noexcept {
    var dirty usz i, j;
    var dirty u8 x;
    for (i = 0, j = len - 1; i < j; i += 1, j -= 1) {
      x = ptr[i];
      ptr[i] = ptr[j];
      ptr[j] = x;
    }
  };

  // Write number (u64) in decimal format to chunk in utf-8 encoding
  //
  // There is no nil check in implementation. Not writing to nil chunk
  // is user's responsibility, doing otherwise will lead to error
  //
  // Returns number of bytes written. If there is not enough space
  // in chunk zero will be returned, but chunk will not be untouched
  method usz fmt_dec(u64 x) noexcept;

  method usz fmt_dec(u32 x) noexcept { return fmt_dec(cast(u64, x)); }

  method usz fmt_dec(u8 x) noexcept;

  // Write number (i64) in decimal format to chunk in utf-8 encoding
  //
  // There is no nil check in implementation. Not writing to nil chunk
  // is user's responsibility, doing otherwise will lead to error
  //
  // Returns number of bytes written. If there is not enough space
  // in chunk zero will be returned, but chunk will not be untouched
  method usz fmt_dec(i64 x) noexcept {
    if (x >= 0) {
      return fmt_dec(cast(u64, x));
    }

    if (len < 2) {
      return 0;
    }
    unsafe_write('-');
    x = -x;
    var usz w = slice_from(1).fmt_dec(cast(u64, x));
    if (w == 0) {
      return 0;
    }
    return w + 1;
  }

  // Same as format_dec, but unused bytes will be filled with padding
  method usz pad_fmt_dec(u64 x) noexcept {
    var usz w = fmt_dec(x);
    if (w == 0) {
      return 0;
    }
    if (w == len) {
      return w;
    }
    slice_from(w).fill(' ');
    return len;
  }

  // Write number (u32) in binary format to chunk in utf-8 encoding.
  // Bytes are separated with single space
  //
  // There is no nil check in implementation. Not writing to nil chunk
  // is user's responsibility, doing otherwise will lead to error
  //
  // Returns number of bytes written. If there is not enough space
  // in chunk zero will be returned, but chunk will not be untouched
  method usz fmt_bin_delim(u32 x) noexcept;

  // Write specified byte repeated n times to memory chunk
  method usz write_repeat(usz n, u8 x) noexcept {
    n = min(n, len);
    unsafe_write_repeat(n, x);
    return n;
  }

  method void unsafe_write_repeat(usz n, u8 x) noexcept {
    for (usz i = 0; i < n; i += 1) {
      ptr[i] = x;
    }
  }

  method usz unsafe_fmt_dec(u64 x) noexcept;
};

typedef mc str;

// shorthand for C string to memory chunk literal conversion
#define macro_static_str(s) str((u8*)u8##s, sizeof(u8##s) - 1)

#define macro_src_file str((u8*)__FILE__, sizeof(__FILE__) - 1)
#define macro_src_line cast(u32, __LINE__)

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

internal const u8 byte_size_decimals_string[] =
    "100"
    "101"
    "102"
    "103"
    "104"
    "105"
    "106"
    "107"
    "108"
    "109"
    "110"
    "111"
    "112"
    "113"
    "114"
    "115"
    "116"
    "117"
    "118"
    "119"
    "120"
    "121"
    "122"
    "123"
    "124"
    "125"
    "126"
    "127"
    "128"
    "129"
    "130"
    "131"
    "132"
    "133"
    "134"
    "135"
    "136"
    "137"
    "138"
    "139"
    "140"
    "141"
    "142"
    "143"
    "144"
    "145"
    "146"
    "147"
    "148"
    "149"
    "150"
    "151"
    "152"
    "153"
    "154"
    "155"
    "156"
    "157"
    "158"
    "159"
    "160"
    "161"
    "162"
    "163"
    "164"
    "165"
    "166"
    "167"
    "168"
    "169"
    "170"
    "171"
    "172"
    "173"
    "174"
    "175"
    "176"
    "177"
    "178"
    "179"
    "180"
    "181"
    "182"
    "183"
    "184"
    "185"
    "186"
    "187"
    "188"
    "189"
    "190"
    "191"
    "192"
    "193"
    "194"
    "195"
    "196"
    "197"
    "198"
    "199"
    "200"
    "201"
    "202"
    "203"
    "204"
    "205"
    "206"
    "207"
    "208"
    "209"
    "210"
    "211"
    "212"
    "213"
    "214"
    "215"
    "216"
    "217"
    "218"
    "219"
    "220"
    "221"
    "222"
    "223"
    "224"
    "225"
    "226"
    "227"
    "228"
    "229"
    "230"
    "231"
    "232"
    "233"
    "234"
    "235"
    "236"
    "237"
    "238"
    "239"
    "240"
    "241"
    "242"
    "243"
    "244"
    "245"
    "246"
    "247"
    "248"
    "249"
    "250"
    "251"
    "252"
    "253"
    "254"
    "255";

struct StringLookupEntry {
  u16 offset;
  u16 len;
};

internal const StringLookupEntry byte_decimal_lookup_table[] = {
    {2, 1},   {5, 1},   {8, 1},   {11, 1},  {14, 1},  {17, 1},  {20, 1},
    {23, 1},  {26, 1},  {29, 1},  {31, 2},  {34, 2},  {37, 2},  {40, 2},
    {43, 2},  {46, 2},  {49, 2},  {52, 2},  {55, 2},  {58, 2},  {61, 2},
    {64, 2},  {67, 2},  {70, 2},  {73, 2},  {76, 2},  {79, 2},  {82, 2},
    {85, 2},  {88, 2},  {91, 2},  {94, 2},  {97, 2},  {100, 2}, {103, 2},
    {106, 2}, {109, 2}, {112, 2}, {115, 2}, {118, 2}, {121, 2}, {124, 2},
    {127, 2}, {130, 2}, {133, 2}, {136, 2}, {139, 2}, {142, 2}, {145, 2},
    {148, 2}, {151, 2}, {154, 2}, {157, 2}, {160, 2}, {163, 2}, {166, 2},
    {169, 2}, {172, 2}, {175, 2}, {178, 2}, {181, 2}, {184, 2}, {187, 2},
    {190, 2}, {193, 2}, {196, 2}, {199, 2}, {202, 2}, {205, 2}, {208, 2},
    {211, 2}, {214, 2}, {217, 2}, {220, 2}, {223, 2}, {226, 2}, {229, 2},
    {232, 2}, {235, 2}, {238, 2}, {241, 2}, {244, 2}, {247, 2}, {250, 2},
    {253, 2}, {256, 2}, {259, 2}, {262, 2}, {265, 2}, {268, 2}, {271, 2},
    {274, 2}, {277, 2}, {280, 2}, {283, 2}, {286, 2}, {289, 2}, {292, 2},
    {295, 2}, {298, 2}, {0, 3},   {3, 3},   {6, 3},   {9, 3},   {12, 3},
    {15, 3},  {18, 3},  {21, 3},  {24, 3},  {27, 3},  {30, 3},  {33, 3},
    {36, 3},  {39, 3},  {42, 3},  {45, 3},  {48, 3},  {51, 3},  {54, 3},
    {57, 3},  {60, 3},  {63, 3},  {66, 3},  {69, 3},  {72, 3},  {75, 3},
    {78, 3},  {81, 3},  {84, 3},  {87, 3},  {90, 3},  {93, 3},  {96, 3},
    {99, 3},  {102, 3}, {105, 3}, {108, 3}, {111, 3}, {114, 3}, {117, 3},
    {120, 3}, {123, 3}, {126, 3}, {129, 3}, {132, 3}, {135, 3}, {138, 3},
    {141, 3}, {144, 3}, {147, 3}, {150, 3}, {153, 3}, {156, 3}, {159, 3},
    {162, 3}, {165, 3}, {168, 3}, {171, 3}, {174, 3}, {177, 3}, {180, 3},
    {183, 3}, {186, 3}, {189, 3}, {192, 3}, {195, 3}, {198, 3}, {201, 3},
    {204, 3}, {207, 3}, {210, 3}, {213, 3}, {216, 3}, {219, 3}, {222, 3},
    {225, 3}, {228, 3}, {231, 3}, {234, 3}, {237, 3}, {240, 3}, {243, 3},
    {246, 3}, {249, 3}, {252, 3}, {255, 3}, {258, 3}, {261, 3}, {264, 3},
    {267, 3}, {270, 3}, {273, 3}, {276, 3}, {279, 3}, {282, 3}, {285, 3},
    {288, 3}, {291, 3}, {294, 3}, {297, 3}, {300, 3}, {303, 3}, {306, 3},
    {309, 3}, {312, 3}, {315, 3}, {318, 3}, {321, 3}, {324, 3}, {327, 3},
    {330, 3}, {333, 3}, {336, 3}, {339, 3}, {342, 3}, {345, 3}, {348, 3},
    {351, 3}, {354, 3}, {357, 3}, {360, 3}, {363, 3}, {366, 3}, {369, 3},
    {372, 3}, {375, 3}, {378, 3}, {381, 3}, {384, 3}, {387, 3}, {390, 3},
    {393, 3}, {396, 3}, {399, 3}, {402, 3}, {405, 3}, {408, 3}, {411, 3},
    {414, 3}, {417, 3}, {420, 3}, {423, 3}, {426, 3}, {429, 3}, {432, 3},
    {435, 3}, {438, 3}, {441, 3}, {444, 3}, {447, 3}, {450, 3}, {453, 3},
    {456, 3}, {459, 3}, {462, 3}, {465, 3},
};

fn internal inline u8 decimal_digit_to_charcode(u8 digit) noexcept {
  return digit + cast(u8, '0');
}

method usz mc::fmt_dec(u8 x) noexcept {
  var StringLookupEntry entry = byte_decimal_lookup_table[x];
  if (len < entry.len) {
    return 0;
  }
  unsafe_write(cast(u8*, byte_size_decimals_string) + entry.offset, entry.len);
  return entry.len;
}

method usz mc::fmt_dec(u64 x) noexcept {
  const usz max_u64_dec_length = 20;
  if (len >= max_u64_dec_length) {
    return unsafe_fmt_dec(x);
  }

  if (x < 10) {
    if (len == 0) {
      return 0;
    }
    ptr[0] = decimal_digit_to_charcode((u8)x);
    return 1;
  }
  if (x < 100) {
    if (len < 2) {
      return 0;
    }
    var u8 offset = cast(u8, x) << 1;
    ptr[0] = small_decimals_string[offset];
    ptr[1] = small_decimals_string[offset + 1];
    return 2;
  }

  var usz i = 0;
  do {  // generate digits in reverse order
    var u8 digit = cast(u8, x % 10);
    x /= 10;
    ptr[i] = decimal_digit_to_charcode(digit);
    i += 1;
  } while (x != 0 && i < len);

  if (x != 0) {
    // chunk does not have enough space to represent given number
    return 0;
  }

  var mc c = mc(ptr, i);
  c.reverse();
  return i;
}

method usz mc::unsafe_fmt_dec(u64 x) noexcept {
  if (x < 10) {
    ptr[0] = decimal_digit_to_charcode((u8)x);
    return 1;
  }
  if (x < 100) {
    var u8 offset = cast(u8, x) << 1;
    ptr[0] = small_decimals_string[offset];
    ptr[1] = small_decimals_string[offset + 1];
    return 2;
  }

  var usz i = 0;
  do {  // generate digits in reverse order
    var u8 digit = cast(u8, x % 10);
    x /= 10;
    ptr[i] = decimal_digit_to_charcode(digit);
    i += 1;
  } while (x != 0);

  var mc c = mc(ptr, i);
  c.reverse();
  return i;
}

method usz mc::fmt_bin_delim(u32 x) noexcept {
  // 32 binary digits + 3 spaces
  const usz l = 8 * sizeof(u32) + 3;
  if (l > len) {
    return 0;
  }

  var usz i = l;
  var dirty u8 digit;
  // digits are written from least to most significant bit
  while (i > l - 8) {
    i += 1;
    digit = x & 1;
    ptr[i] = decimal_digit_to_charcode(digit);
    x = x >> 1;
  }

  i += 1;
  ptr[i] = ' ';

  while (i > l - 17) {
    i += 1;
    digit = x & 1;
    ptr[i] = decimal_digit_to_charcode(digit);
    x = x >> 1;
  }

  i += 1;
  ptr[i] = ' ';

  while (i > l - 26) {
    i += 1;
    digit = x & 1;
    ptr[i] = decimal_digit_to_charcode(digit);
    x = x >> 1;
  }

  i += 1;
  ptr[i] = ' ';

  while (i > 1) {
    i += 1;
    digit = x & 1;
    ptr[i] = decimal_digit_to_charcode(digit);
    x = x >> 1;
  }

  ptr[0] = decimal_digit_to_charcode(cast(u8, x));

  return l;
}

namespace fmt {

fn inline u8 number_to_hex_digit(u8 x) noexcept {
  if (x <= 9) {
    return x + cast(u8, '0');
  }
  return x - 0xA + cast(u8, 'A');
}

// Formats a given u8 integer as a hexadecimal number in
// exactly two digits. Memory chunk must be at least 2 bytes
// long
fn void unsafe_byte(mc c, u8 x) noexcept {
  const u8 d0 = number_to_hex_digit(x & 0xF);
  const u8 d1 = number_to_hex_digit(x >> 4);
  c.ptr[0] = d1;
  c.ptr[1] = d0;
}

// Memory chunk must be at least 4 bytes long
fn void unsafe_hex_prefix_byte(mc c, u8 x) noexcept {
  c.ptr[0] = '0';
  c.ptr[1] = 'x';
  unsafe_byte(c.slice_from(2), x);
}

}  // namespace fmt

// Two Memory Chunks are considered equal if they have
// identical length and bytes content
fn bool compare_equal(mc a, mc b) noexcept {
  if (a.len != b.len) {
    return false;
  }

  const usz len = a.len;

  for (usz i = 0; i < len; i += 1) {
    if (a.ptr[i] != b.ptr[i]) {
      return false;
    }
  }

  return true;
}

// Bytes Buffer
//
// Accumulates multiple writes into single continuous memory region.
// Buffer has capacity i.e. maximum number of bytes it can hold. After
// number of written bytes reaches this maximum all subsequent writes
// will write nothing and always return 0, until reset() is called
//
// Buffer is much alike memory chunk in terms of memory ownership
struct bb {
  u8* ptr;

  // number of bytes currently stored in buffer
  usz len;

  // buffer capacity i.e. maximum number of bytes it can hold
  usz cap;

  let constexpr bb() noexcept : ptr(nil), len(0), cap(0) {}

  let bb(mc c) noexcept : ptr(c.ptr), len(0), cap(c.len) {}

  let bb(u8* bytes, usz n) noexcept : ptr(bytes), len(0), cap(n) {}

  method bool is_empty() noexcept { return len == 0; }

  method bool is_full() noexcept { return len == cap; }

  method bool is_nil() noexcept { return cap == 0; }

  // Returns number of bytes which can be written to buffer before it is full
  method usz rem() noexcept { return cap - len; }

  method void reset() noexcept { len = 0; }

  // Returns pointer to first byte at buffer position
  method u8* tip() noexcept { return ptr + len; }

  method usz write(u8* bytes, usz n) noexcept {
    var usz w = min(n, rem());
    if (w == 0) {
      return 0;
    }
    copy(bytes, tip(), w);
    len += w;
    return w;
  }

  method usz write(u8 b) noexcept {
    if (is_full()) {
      return 0;
    }
    ptr[len] = b;
    len += 1;
    return 1;
  }

  // add line feed (aka "newline") character to buffer
  method usz lf() noexcept { return write('\n'); }

  method usz write(mc c) noexcept { return write(c.ptr, c.len); }

  // Write specified byte repeated n times to memory chunk
  method usz write_repeat(usz n, u8 x) noexcept {
    var mc t = tail();
    const usz k = t.write_repeat(n, x);
    len += k;
    return k;
  }

  method void unsafe_write_repeat(usz n, u8 x) noexcept {
    var mc t = tail();
    t.unsafe_write_repeat(n, x);
    len += n;
  }

  method usz unsafe_write(mc c) noexcept {
    copy(c.ptr, tip(), c.len);
    len += c.len;
    return c.len;
  }

  // Use for writing character literals like this
  //
  // buf.unsafe_write('L');
  method void unsafe_write(char x) noexcept { unsafe_write(cast(u8, x)); }

  method void unsafe_write(u8 x) noexcept {
    ptr[len] = x;
    len += sizeof(u8);
  }

  method void unsafe_write(i16 x) noexcept {
    var i16* p = cast(i16*, tip());
    *p = x;
    len += sizeof(i16);
  }

  method void unsafe_write(f32 x) noexcept {
    var f32* p = cast(f32*, tip());
    *p = x;
    len += sizeof(f32);
  }

  // Insert single byte at specified index. All bytes with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  //
  // For correct behaviour the following inequation must be
  // satisfied before the call:
  //
  // i <= len < cap
  //
  // In the special case of i == len new byte will be appended
  // at the tip of buffer
  //
  // This method does not perform any safety checks
  method void unsafe_insert(usz i, u8 x) noexcept {
    if (i == len) {
      unsafe_write(x);
      return;
    }

    move(ptr + i, ptr + i + 1, len - i);
    len += 1;
    ptr[i] = x;
  }

  method usz fmt_dec(u8 x) noexcept {
    var mc t = tail();
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  // overload for convenient usage with sizeof() results
  method usz fmt_dec(unsigned long int x) noexcept {
    return fmt_dec(cast(u64, x));
  }

  method usz fmt_dec(u16 x) noexcept { return fmt_dec(cast(u64, x)); }

  method usz fmt_dec(u32 x) noexcept { return fmt_dec(cast(u64, x)); }

  method usz unsafe_fmt_dec(u32 x) noexcept {
    return unsafe_fmt_dec(cast(u64, x));
  }

  method usz unsafe_fmt_dec(u64 x) noexcept {
    var mc t = tail();
    var usz w = t.unsafe_fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_dec(u64 x) noexcept {
    if (is_full()) {
      return 0;
    }

    var mc t = tail();
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_dec(i64 x) noexcept {
    var mc t = tail();
    if (t.is_nil()) {
      return 0;
    }
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_bin_delim(u32 x) noexcept {
    var mc t = tail();
    var usz w = t.fmt_bin_delim(x);
    len += w;
    return w;
  }

  // Returns memory chunk which is occupied by actual data
  // inside buffer
  method mc head() noexcept { return mc(ptr, len); }

  // Returns memory chunk which is a portion of buffer
  // that is available for writes
  method mc tail() noexcept { return mc(tip(), rem()); }

  // Returns full body (from zero to capacity) of buffer
  method mc body() noexcept { return mc(ptr, cap); }
};

fn void stdout_write(mc c) noexcept;

fn void stderr_write(mc c) noexcept;

fn void stdout_write_all(mc c) noexcept;

fn void stderr_write_all(mc c) noexcept;

// Describes location in source code
struct src_loc {
  mc file;
  u32 line;

  let src_loc(mc f, u32 l) noexcept : file(f), line(l) {}

  method usz fmt(mc c) noexcept {
    var bb buf = bb(c);

    buf.write(file);
    buf.write(':');
    buf.fmt_dec(line);

    return buf.len;
  }

  method void fmt_to_stderr() noexcept {
    var dirty u8 b[16];
    var bb buf = bb(b, sizeof(b));
    buf.write(':');
    buf.fmt_dec(line);

    stderr_write_all(file);
    stderr_write_all(buf.head());
  }
};

#define macro_src_loc() src_loc(macro_src_file, macro_src_line)

[[noreturn]] fn void check_unreachable(src_loc loc) noexcept {
  loc.fmt_to_stderr();
  stderr_write_all(macro_static_str(" <-- touched unreachable code\n"));

  __builtin_unreachable();
}

fn void check_must(bool condition, src_loc loc) noexcept {
  if (condition) {
    return;
  }

  loc.fmt_to_stderr();
  stderr_write_all(macro_static_str(" <-- assert failed\n"));

  abort();
}

#define unreachable() check_unreachable(macro_src_loc())

#define must(x) check_must(x, macro_src_loc())

[[noreturn]] fn void fatal(mc msg, src_loc loc) noexcept;

#define panic(msg) fatal(msg, macro_src_loc())

// not implemented macro
#define noimp() panic(macro_static_str("not implemented"))

// Stores elements (of the same type) in continuous memory region
// and their number together as a single data structure
template <typename T>
struct chunk {
  // Pointer to first element in chunk
  T* ptr;

  // Number of elements in chunk
  usz len;

  let chunk() noexcept : ptr(nil), len(0) {}

  let chunk(T* elems, usz n) noexcept : ptr(elems), len(n) {}

  method bool is_nil() noexcept { return len == 0; }

  method mc as_mc() noexcept { return mc(cast(u8*, ptr), size()); }

  method usz size() noexcept { return chunk_size(T, len); }

  method void clear() noexcept { as_mc().clear(); }
};

// Buffer for storing elements (of the same type) in continuous memory region
// which allows appending new elements to the end of buffer until buffer
// capacity is reached
template <typename T>
struct buffer {
  // Pointer to buffer starting position
  T* ptr;

  // Number of elements stored in buffer
  usz len;

  // Buffer capacity i.e. maximum number of elements it can hold
  usz cap;

  let constexpr buffer() noexcept : ptr(nil), len(0), cap(0) {}

  let buffer(chunk<T> c) noexcept : ptr(c.ptr), len(0), cap(c.len) {}

  let buffer(T* p, usz n) noexcept : ptr(p), len(0), cap(n) {}

  method bool is_empty() noexcept { return len == 0; }

  method bool is_full() noexcept { return len == cap; }

  method bool is_nil() noexcept { return cap == 0; }

  // Returns number of elements which can be appended to buffer before it is
  // full
  method usz rem() noexcept { return cap - len; }

  method void reset() noexcept { len = 0; }

  // Returns chunk which is occupied by actual data
  // inside buffer
  method chunk<T> head() noexcept { return chunk<T>(ptr, len); }

  // Append new element to buffer tail
  method usz append(T elem) noexcept {
    if (is_full()) {
      return 0;
    }

    unsafe_append(elem);
    return 1;
  }

  method void unsafe_append(T elem) noexcept {
    ptr[len] = elem;
    len += 1;
  }

  method chunk<T> body() noexcept { return chunk<T>(ptr, cap); }
};

namespace bits {

fn inline bool is_aligned_16(anyptr x) noexcept {
  return (cast(uptr, x) & 0x0F) == 0;
}

fn inline bool is_aligned_16(usz x) noexcept {
  return (x & 0x0F) == 0;
}

fn inline usz align_16(usz x) noexcept {
  usz a = x & 0x0F;
  a = ((~a) + 1) & 0x0F;
  return x + a;
}

}  // namespace bits

namespace mem {

struct Arena {
  // Internal buffer which is used for allocating chunks
  mc buf;

  // Position of next chunk to be allocated. Can also be
  // interpreted as total amount of bytes already used
  // inside arena
  usz pos;

  // Previous value of pos field. Used for popping previously
  // allocated chunk
  // usz prev;

  let Arena(mc c) noexcept : buf(c), pos(0) {
    must(!c.is_nil());
    must(bits::is_aligned_16(c.ptr));
  }

  // Allocate at least n bytes of memory
  //
  // Returns allocated memory chunk. Note that its length
  // may be bigger than number of bytes requested
  method mc alloc(usz n) noexcept {
    must(n != 0);

    n = bits::align_16(n);
    must(n <= rem());

    const usz prev = pos;
    pos += n;

    return buf.slice(prev, pos);
  }

  // Allocate a non-overlapping copy of given memory chunk.
  // In contrast with alloc method returned chunk will be
  // of exactly the same length as original one
  //
  // Clients cannot pop back the memory allocated via
  // this method
  method mc allocate_copy(mc c) noexcept {
    var mc cp = alloc(c.len);
    cp.unsafe_write(c);
    return cp.slice_to(c.len);
  }

  method usz rem() noexcept { return buf.len - pos; }

  // Pop n bytes from previous allocations, marking them as
  // available for next allocations. Popped memory will no
  // longer be valid for clients who aliased it
  //
  // Use this operation with extreme caution. It is primary usage
  // intended for deallocating short-lived scratch buffers which
  // lifetime is limited by narrow scope
  method void pop(usz n) noexcept {
    must(n != 0);
    must(bits::is_aligned_16(n));
    must(n <= pos);

    pos -= n;
  }

  // Drops all allocated memory and makes entire buffer
  // available for future allocations
  method void reset() noexcept { pos = 0; }
};

struct Gen {
  let Gen() noexcept {}

  method mc alloc(usz n) noexcept {
    must(n != 0);

    var u8* ptr = cast(u8*, malloc(n));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  // Allocate a non-overlapping copy of given memory chunk
  method mc allocate_copy(mc c) noexcept {
    var mc cp = alloc(c.len);
    cp.unsafe_write(c);
    return cp;
  }

  template <typename T>
  method chunk<T> alloc(usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(n != 0);

    var T* ptr = cast(T*, malloc(chunk_size(T, n)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method mc calloc(usz n) noexcept {
    must(n != 0);

    var u8* ptr = cast(u8*, ::calloc(n, 1));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  template <typename T>
  method chunk<T> calloc(usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(n != 0);

    var T* ptr = cast(T*, ::calloc(n, sizeof(T)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method mc realloc(mc c, usz n) noexcept {
    must(c.ptr != nil && c.len != 0 && n != 0 && c.len != n);

    var u8* ptr = cast(u8*, ::realloc(c.ptr, n));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  template <typename T>
  method chunk<T> realloc(chunk<T> c, usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(c.ptr != nil && c.len != 0 && n != 0 && c.len != n);

    var T* ptr = cast(T*, ::realloc(c.ptr, chunk_size(T, n)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method void free(mc c) noexcept {
    must(c.ptr != nil && c.len != 0);

    ::free(c.ptr);
  }

  template <typename T>
  method void free(chunk<T> c) noexcept {
    must(c.ptr != nil && c.len != 0);

    ::free(c.ptr);
  }
};

var global Gen gpa_global_instance = Gen();

fn mc alloc(usz n) noexcept {
  return gpa_global_instance.alloc(n);
}

template <typename T>
fn chunk<T> alloc(usz n) noexcept {
  return gpa_global_instance.alloc<T>(n);
}

fn mc calloc(usz n) noexcept {
  return gpa_global_instance.calloc(n);
}

template <typename T>
fn chunk<T> calloc(usz n) noexcept {
  return gpa_global_instance.calloc<T>(n);
}

fn mc realloc(mc c, usz n) noexcept {
  return gpa_global_instance.realloc(c, n);
}

template <typename T>
fn chunk<T> realloc(chunk<T> c, usz n) noexcept {
  return gpa_global_instance.realloc<T>(c, n);
}

fn void free(mc c) noexcept {
  gpa_global_instance.free(c);
}

template <typename T>
fn void free(chunk<T> c) noexcept {
  gpa_global_instance.free<T>(c);
}

struct alc {
  enum struct Kind : u8 {
    Nil,
    Arena,
    Gen,
  };

  anyptr ptr;

  Kind kind;

  let alc() noexcept : ptr(nil), kind(Kind::Nil) {}

  let alc(Arena* arena) noexcept
      : ptr(cast(anyptr, arena)), kind(Kind::Arena) {}

  let alc(Gen* gen) noexcept : ptr(cast(anyptr, gen)), kind(Kind::Gen) {}

  method bool is_nil() noexcept { return kind == Kind::Nil; }

  method mc alloc(usz n) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        return gen->alloc(n);
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }

  method mc realloc(mc c, usz n) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        return gen->realloc(c, n);
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }

  method void free(mc c) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        gen->free(c);
        return;
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }
};

}  // namespace mem

fn internal inline usz determine_bytes_grow_amount(usz cap, usz len) noexcept {
  if (len <= cap) {
    const usz size_four_megabytes = 1 << 22;
    if (cap < size_four_megabytes) {
      return cap;
    }

    return len + (cap >> 10);
  }

  if (cap != 0) {
    return len + (cap >> 1);
  }

  if (len < 16) {
    return 16;
  }

  if (len < 64) {
    return 64;
  }

  return len + (len >> 1);
}

// Dynamically allocated bytes buffer. Default global general
// purpose allocator is used to manage underlying memory
struct DynBytesBuffer {
  bb buf;

  let DynBytesBuffer() noexcept : buf(bb()) {}

  let DynBytesBuffer(usz n) noexcept : buf(bb()) { init(n); }

  // Allocates a dynamic copy of a given memory chunk
  let DynBytesBuffer(mc c) noexcept : buf(bb()) {
    if (c.is_nil()) {
      return;
    }
    init(c.len);
    buf.unsafe_write(c);
  }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = bb(mem::alloc(n));
  }

  method bool is_nil() noexcept { return buf.is_nil(); }

  // Increase buffer capacity by at least n bytes
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = bb(mem::alloc(n));
      return;
    }

    const usz len = buf.len;

    buf = bb(mem::realloc(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void write(mc c) noexcept {
    if (c.is_nil()) {
      return;
    }

    if (buf.rem() >= c.len) {
      buf.unsafe_write(c);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, c.len);
    grow(n);

    buf.unsafe_write(c);
  }

  // Insert single byte at specified index. All bytes with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  method void insert(usz i, u8 x) noexcept {
    must(i <= buf.len);

    if (buf.rem() >= 1) {
      buf.unsafe_insert(i, x);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_insert(i, x);
  }

  method void write(u8 x) noexcept {
    if (buf.rem() >= 1) {
      buf.unsafe_write(x);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_write(x);
  }

  method void write_repeat(usz n, u8 x) noexcept {
    if (buf.rem() >= n) {
      buf.unsafe_write_repeat(n, x);
      return;
    }

    const usz amount = determine_bytes_grow_amount(buf.cap, n);
    grow(amount);

    buf.unsafe_write_repeat(n, x);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    mem::free(buf.body());
  }

  method void reset() noexcept { buf.reset(); }

  method mc head() noexcept { return buf.head(); }
};

// Dynamically allocated bytes buffer
struct dbb {
  bb buf;

  // Allocator used to pull/release memory for buffer
  mem::alc alc;

  let dbb() noexcept : buf(bb()), alc(mem::alc()) {}

  let dbb(mem::alc a) noexcept : buf(bb()), alc(a) {}

  let dbb(mem::alc a, usz n) noexcept : buf(bb()), alc(a) { init(n); }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = bb(alc.alloc(n));
  }

  method bool is_nil() noexcept { return alc.is_nil(); }

  // Increase buffer capacity by at least n bytes
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = bb(alc.alloc(n));
      return;
    }

    const usz len = buf.len;

    buf = bb(alc.realloc(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void write(mc c) noexcept {
    if (c.is_nil()) {
      return;
    }

    if (buf.rem() >= c.len) {
      buf.unsafe_write(c);
      return;
    }

    // calculate by how much we need to grow buffer capacity
    var dirty usz n;
    if (c.len <= buf.cap) {
      n = buf.cap;
    } else if (buf.cap != 0) {
      n = c.len + (buf.cap >> 1);
    } else {
      if (c.len < 16) {
        n = 16;
      } else if (c.len < 64) {
        n = 64;
      } else {
        n = c.len + (c.len >> 1);
      }
    }

    grow(n);

    buf.unsafe_write(c);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    alc.free(buf.body());
  }
};

fn internal inline usz determine_grow_amount(usz cap, usz len) noexcept {
  if (len <= cap) {
    const usz size_threshold = 1 << 16;
    if (cap < size_threshold) {
      return cap;
    }

    return len + (cap >> 8);
  }

  if (cap != 0) {
    return len + (cap >> 1);
  }

  if (len < 16) {
    return 16;
  }

  if (len < 64) {
    return 64;
  }

  return len + (len >> 1);
}

// Dynamically allocated buffer of elements (of the same type).
// It is most similar to dynamic arrays from other libraries or
// languages. Default global general purpose allocator is used
// to manage underlying memory
template <typename T>
struct DynBuffer {
  buffer<T> buf;

  let DynBuffer() noexcept : buf(buffer<T>()) {}

  let DynBuffer(usz n) noexcept : buf(buffer<T>()) { init(n); }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = buffer<T>(mem::alloc<T>(n));
  }

  // Increase buffer capacity by at least n elements
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = buffer<T>(mem::alloc<T>(n));
      return;
    }

    const usz len = buf.len;

    buf = buffer<T>(mem::realloc<T>(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void append(T elem) noexcept {
    if (buf.rem() >= 1) {
      buf.unsafe_append(elem);
      return;
    }

    const usz n = determine_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_append(elem);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    mem::free(buf.body());
  }

  method void reset() noexcept { buf.reset(); }

  method usz len() noexcept { return buf.len; }

  method mc head() noexcept { return buf.head(); }
};

// C String
//
// Represents a sequence of bytes in which the last byte always equals
// zero. Designed for interfacing with traditional C-style strings
struct cstr {
  // Pointer to first byte in string
  u8* ptr;

  // Number of bytes in string before zero terminator byte
  //
  // Thus actual number of stored bytes available through
  // pointer is len + 1
  usz len;

  let cstr() noexcept : ptr(nil), len(0) {}

  // This constructor is deliberately unsafe. Use it only
  // on C strings which come from trusted source
  //
  // For safe version with proper error-checking refer to
  // function try_parse_cstr
  let cstr(u8* s) noexcept {
    var usz i = 0;
    while (s[i] != 0) {
      i += 1;
    }
    ptr = s;
    len = i;
  }

  // Use this constructor if length of C string is already
  // known before the call. Second argument is number of
  // bytes in string, not including zero terminator
  let cstr(u8* s, usz n) noexcept : ptr(s), len(n) {}

  method bool is_nil() noexcept { return len == 0; }

  // Returns Memory Chunk with stored bytes. Zero terminator
  // is not included
  method mc as_str() noexcept { return mc(ptr, len); }

  // Returns Memory Chunk with stored bytes. Includes zero terminator
  // in its length
  method mc with_term() noexcept { return mc(ptr, len + 1); }
};

#define macro_static_cstr(s) cstr((u8*)u8##s, sizeof(u8##s) - 1)

struct FileReadResult {
  enum struct Code : u8 {
    Ok,

    // Generic error, no specifics known
    Error,

    AlreadyExists,
  };

  mc data;

  Code code;

  let FileReadResult(mc d) noexcept : data(d), code(Code::Ok) {}
  let FileReadResult(Code c) noexcept : data(mc()), code(c) {}
  let FileReadResult(mc d, Code c) noexcept : data(d), code(c) {}

  method bool is_ok() noexcept { return code == Code::Ok; }
  method bool is_err() noexcept { return code != Code::Ok; }
};

namespace fs {

struct ReadResult {
  enum struct Code : u32 {
    Ok,

    EOF,

    // Generic error, no specifics known
    Error,
  };

  // Number of bytes read. May be not 0 even if code != Ok
  usz n;

  Code code;

  let ReadResult() noexcept : n(0), code(Code::Ok) {}
  let ReadResult(usz k) noexcept : n(k), code(Code::Ok) {}
  let ReadResult(Code c) noexcept : n(0), code(c) {}
  let ReadResult(Code c, usz k) noexcept : n(k), code(c) {}

  method bool is_ok() noexcept { return code == Code::Ok; }
  method bool is_eof() noexcept { return code == Code::EOF; }
  method bool is_err() noexcept {
    return code != Code::Ok && code != Code::EOF;
  }
};

struct WriteResult {
  enum struct Code : u32 {
    Ok,

    // Generic error, no specifics known
    Error,
  };

  // Number of bytes written
  usz n;

  Code code;

  let WriteResult() noexcept : n(0), code(Code::Ok) {}
  let WriteResult(usz k) noexcept : n(k), code(Code::Ok) {}
  let WriteResult(Code c) noexcept : n(0), code(c) {}
  let WriteResult(Code c, usz k) noexcept : n(k), code(c) {}

  method bool is_ok() noexcept { return code == Code::Ok; }
  method bool is_err() noexcept { return code != Code::Ok; }
};

typedef usz FileDescriptor;

fn ReadResult read(FileDescriptor fd, mc c) noexcept;

fn ReadResult read_all(FileDescriptor fd, mc c) noexcept {
  var usz i = 0;
  while (i < c.len) {
    var ReadResult r = read(fd, c.slice_from(i));
    i += r.n;

    if (!r.is_ok()) {
      return ReadResult(r.code, i);
    }
  }

  return ReadResult(c.len);
}

fn WriteResult write(FileDescriptor fd, mc c) noexcept;

fn WriteResult write_all(FileDescriptor fd, mc c) noexcept {
  var usz i = 0;
  while (i < c.len) {
    var WriteResult r = write(fd, c.slice_from(i));
    i += r.n;

    if (r.is_err()) {
      return WriteResult(r.code, i);
    }
  }

  return WriteResult(c.len);
}

struct BufFileWriter {
  // Internal buffer for storing raw bytes before
  // commiting accumulated writes to file
  bb buf;

  // File descriptor
  //
  // Actual meaning of this number is platform-specific
  FileDescriptor fd;

  let BufFileWriter(FileDescriptor f, mc c) noexcept : buf(bb(c)), fd(f) {}

  method WriteResult write(mc c) noexcept {
    var usz i = 0;
    while (i < c.len) {
      var usz n = buf.write(c.slice_from(i));
      i += n;
      if (n == 0) {
        var WriteResult r = flush();
        if (r.is_err()) {
          return WriteResult(r.code, i);
        }
      }
    }

    return WriteResult(c.len);
  }

  // Commits stored writes to underlying file (designated by file descriptor)
  method WriteResult flush() noexcept {
    var WriteResult r = write_all(fd, buf.head());
    if (r.is_err()) {
      return r;
    }

    buf.reset();
    return r;
  }
};

}  // namespace fs

namespace hash {

internal const u64 offset_fnv64 = 14695981039346656037ULL;
internal const u64 prime_fnv64 = 1099511628211;

struct fnv64a {
  u64 sum;

  let constexpr fnv64a() noexcept : sum(offset_fnv64) {}

  method void reset() noexcept { sum = offset_fnv64; }

  method void write(mc c) noexcept {
    var u64 hash = sum;
    for (usz i = 0; i < c.len; i += 1) {
      hash ^= cast(u64, c.ptr[i]);
      hash *= prime_fnv64;
    }
    sum = hash;
  }

  method void write(u8 x) noexcept {
    var u64 hash = sum;
    hash ^= cast(u64, x);
    hash *= prime_fnv64;
    sum = hash;
  }
};

namespace djb2 {

internal const u64 magic = 5381;

fn u64 compute(mc c) noexcept {
  var u64 h = magic;
  for (usz i = 0; i < c.len; i += 1) {
    h = ((h << 5) + h) + c.ptr[i];
  }
  return h;
}

}  // namespace djb2

namespace map {

// This implementation is a port from Go source code
// Original can be found at hash/maphash/maphash_purego.go

fn internal u32 le_u32(mc c) noexcept {
  // must be c.len >= 4
  return cast(u32, c.ptr[0]) | (cast(u32, c.ptr[1]) << 8) |
         (cast(u32, c.ptr[2]) << 16) | (cast(u32, c.ptr[3]) << 24);
}

fn internal u64 r3(u64 k, mc c) noexcept {
  return (cast(u64, c.ptr[0]) << 16) | (cast(u64, c.ptr[k >> 1]) << 8) |
         cast(u64, c.ptr[k - 1]);
}

fn internal u64 r4(mc c) noexcept {
  return cast(u64, le_u32(c));
}

fn internal u64 le_u64(mc c) noexcept {
  // must be c.len >= 8
  return cast(u64, c.ptr[0]) | (cast(u64, c.ptr[1]) << 8) |
         (cast(u64, c.ptr[2]) << 16) | (cast(u64, c.ptr[3]) << 24) |
         (cast(u64, c.ptr[4]) << 32) | (cast(u64, c.ptr[5]) << 40) |
         (cast(u64, c.ptr[6]) << 48) | (cast(u64, c.ptr[7]) << 56);
}

fn internal u64 r8(mc c) noexcept {
  return le_u64(c);
}

fn internal u64 mix(u64 a, u64 b) noexcept {
  const u128 n = cast(u128, a) * cast(u128, b);
  const u64 lo = cast(u64, n);
  const u64 hi = cast(u64, (n >> 64));

  return hi ^ lo;
}

internal const u64 m1 = 0xa0761d6478bd642f;
internal const u64 m2 = 0xe7037ed1a0b428db;
internal const u64 m3 = 0x8ebc6af09c88c6e3;
internal const u64 m4 = 0x589965cc75374cc3;
internal const u64 m5 = 0x1d8e4e27c47d124f;

fn internal u64 wyhash(u64 seed, mc c) noexcept {
  const u64 len = c.len;

  seed ^= m1;
  var u64 i = len;
  if (i > 16) {
    if (i > 48) {
      var u64 seed1 = seed;
      var u64 seed2 = seed;
      for (; i > 48; i -= 48) {
        seed = mix(r8(c) ^ m2, r8(c.slice_from(8)) ^ seed);
        seed1 = mix(r8(c.slice_from(16)) ^ m3, r8(c.slice_from(24)) ^ seed1);
        seed2 = mix(r8(c.slice_from(32)) ^ m4, r8(c.slice_from(40)) ^ seed2);
        c = c.slice_from(48);
      }
      seed ^= seed1 ^ seed2;
    }

    for (; i > 16; i -= 16) {
      seed = mix(r8(c) ^ m2, r8(c.slice_from(8)) ^ seed);
      c = c.slice_from(16);
    }
  }

  if (i == 0) {
    return seed;
  }

  var u64 a = 0;
  var u64 b = 0;

  if (i < 4) {
    a = r3(i, c);
  } else {
    const u64 n = (i >> 3) << 2;
    a = (r4(c) << 32) | r4(c.slice_from(n));
    b = (r4(c.slice_from(i - 4)) << 32) | r4(c.slice_from(i - 4 - n));
  }

  return mix(m5 ^ len, mix(a ^ m2, b ^ seed));
}

fn internal u64 rthash(u64 seed, mc c) noexcept {
  if (c.len == 0) {
    return seed;
  }
  return wyhash(seed, c);
}

internal const u64 buf_size = 128;

fn u64 compute(u64 seed, mc c) noexcept {
  while (c.len > buf_size) {
    seed = rthash(seed, c.slice_to(buf_size));
    c = c.slice_from(buf_size);
  }
  return rthash(seed, c);
}

}  // namespace map

}  // namespace hash

namespace debug {

// caller should supply memory chunk of at least 256 bytes long
fn mc print_types_info(mc c) noexcept {
  var bb buf = bb(c);

  buf.write(macro_static_str("\nu8\t\t"));
  buf.fmt_dec(sizeof(u8));

  buf.write(macro_static_str("\nu16\t\t"));
  buf.fmt_dec(sizeof(u16));

  buf.write(macro_static_str("\nu32\t\t"));
  buf.fmt_dec(sizeof(u32));

  buf.write(macro_static_str("\nu64\t\t"));
  buf.fmt_dec(sizeof(u64));

  buf.write(macro_static_str("\nu128\t\t"));
  buf.fmt_dec(sizeof(u128));

  buf.lf();

  buf.write(macro_static_str("\ni8\t\t"));
  buf.fmt_dec(sizeof(i8));

  buf.write(macro_static_str("\ni16\t\t"));
  buf.fmt_dec(sizeof(i16));

  buf.write(macro_static_str("\ni32\t\t"));
  buf.fmt_dec(sizeof(i32));

  buf.write(macro_static_str("\ni64\t\t"));
  buf.fmt_dec(sizeof(i64));

  buf.write(macro_static_str("\ni128\t\t"));
  buf.fmt_dec(sizeof(i128));

  buf.lf();

  buf.write(macro_static_str("\nf32\t\t"));
  buf.fmt_dec(sizeof(f32));

  buf.write(macro_static_str("\nf64\t\t"));
  buf.fmt_dec(sizeof(f64));

  buf.write(macro_static_str("\nf128\t\t"));
  buf.fmt_dec(sizeof(f128));

  buf.lf();

  buf.write(macro_static_str("\nusz\t\t"));
  buf.fmt_dec(sizeof(usz));

  buf.write(macro_static_str("\nisz\t\t"));
  buf.fmt_dec(sizeof(isz));

  buf.write(macro_static_str("\nuptr\t\t"));
  buf.fmt_dec(sizeof(void*));

  buf.lf();

  buf.write(macro_static_str("\nmc\t\t"));
  buf.fmt_dec(sizeof(mc));

  buf.write(macro_static_str("\nbb\t\t"));
  buf.fmt_dec(sizeof(bb));

  buf.lf();

  return buf.head();
}

}  // namespace debug

namespace text {

fn inline bool is_latin_letter(rune r) noexcept {
  r = r & 0xFFFFFFDF;
  return 'A' <= r && r <= 'Z';
}

fn inline bool is_latin_letter_or_underscore(rune r) noexcept {
  return is_latin_letter(r) || r == '_';
}

fn inline bool is_decimal_digit(rune r) noexcept {
  return '0' <= r && r <= '9';
}

fn inline bool is_alphanum(rune r) noexcept {
  return is_latin_letter_or_underscore(r) || is_decimal_digit(r);
}

fn inline bool is_decimal_digit_or_period(rune r) noexcept {
  return is_decimal_digit(r) || r == '.';
}

fn inline bool is_simple_whitespace(rune r) noexcept {
  return r == ' ' || r == '\n' || r == '\t' || r == '\r';
}

fn inline bool is_hexadecimal_digit(rune r) noexcept {
  const rune h = r & 0xFFFFFFDF;
  return is_decimal_digit(r) || ('A' <= h && h <= 'F');
}

fn inline bool is_octal_digit(rune r) noexcept {
  return '0' <= r && r <= '7';
}

fn inline bool is_binary_digit(rune r) noexcept {
  return r == '0' || r == '1';
}

}  // namespace text

namespace bits {

fn inline u32 upper_power_of_two(u32 v) noexcept {
  v -= 1;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v += 1;
  return v;
}

}  // namespace bits

namespace strconv {

fn inline u8 dec_digit_to_number(u8 c) noexcept {
  return c - cast(u8, '0');
}

fn inline u8 hex_digit_to_number(u8 c) noexcept {
  if (c <= '9') {
    return dec_digit_to_number(c);
  }

  // transforms small into capital letters
  c = c & 0xDF;

  return c - 'A' + 0x0A;
}

fn u64 unsafe_parse_dec(str s) noexcept {
  var u64 n = 0;
  for (usz i = 0; i < s.len; i += 1) {
    n *= 10;
    n += dec_digit_to_number(s.ptr[i]);
  }
  return n;
}

fn u64 unsafe_parse_bin(str s) noexcept {
  var u64 v = 0;
  for (usz i = 0; i < s.len; i += 1) {
    v <<= 1;
    v += dec_digit_to_number(s.ptr[i]);
  }
  return v;
}

fn u64 unsafe_parse_oct(str s) noexcept {
  var u64 v = 0;
  for (usz i = 0; i < s.len; i += 1) {
    v <<= 3;
    v += dec_digit_to_number(s.ptr[i]);
  }
  return v;
}

fn u64 unsafe_parse_hex(str s) noexcept {
  var u64 v = 0;
  for (usz i = 0; i < s.len; i += 1) {
    v <<= 4;
    v += hex_digit_to_number(s.ptr[i]);
  }
  return v;
}

}  // namespace strconv

namespace container {

// Hash table of static size with no collisions. This is achived
// by handpicking starting seed for hash function
template <typename T>
struct FlatMap {
  // Key-value pair that can be added to map. Mainly used
  // to populate a map with many values at once
  struct Pair {
    str key;
    T value;

    let Pair(str k, T v) noexcept : key(k), value(v) {}
  };

  // Item stored in map. Returned when get method is used
  struct Item {
    // value that was placed in map by add method
    T value;

    // true if item is actually stored in map, i.e.
    // it is map item occupation flag
    bool ok;

    // constructs empty item
    let Item() noexcept : value(T()), ok(false) {}

    let Item(T v) noexcept : value(v), ok(true) {}
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

  method u64 hash(str key) noexcept { return hash::map::compute(seed, key); }

  method usz determine_pos(u64 h) noexcept { return h & mask; }

  // Returns true if item was successfully added to map.
  // Returns false if corresponding cell was already
  // occupied in map
  method bool add(str key, T value) noexcept {
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

  method bool add(Pair pair) noexcept { return add(pair.key, pair.value); }

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
  fn bool populate(chunk<Pair> pairs) noexcept {
    for (usz i = 0; i < pairs.len; i += 1) {
      const Pair pair = pairs.ptr[i];
      const bool ok = add(pair);

      if (!ok) {
        return false;
      }
    }

    return true;
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
template <typename T>
fn FlatMap<T> fit_into_flat_map(
    usz cap,
    u64 mask,
    chunk<typename FlatMap<T>::Pair> pairs) noexcept {
  //
  var FlatMap<T> m = FlatMap<T>(cap, mask, 0);

  for (u64 j = 0; j < 100000; j += 1) {
    const bool ok = m.populate(pairs);

    if (ok) {
      return m;
    }

    m.clear();
  }

  m.free();
  return FlatMap<T>();
}

}  // namespace container

#endif  // GUARD_CORE_HPP
