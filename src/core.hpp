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

// Dummy usage for variable, argument or constant
//
// Suppresses compiler warnings
#define nop_use(x) (void)(x)

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long long int i64;

typedef float f32;
typedef double f64;

// Platform dependent unsigned integer type. Always have enough bytes to
// represent size of any memory region (or offset in memory region)
typedef u64 usz;
typedef i64 isz;

// Type for performing arbitrary arithmetic pointer operations
typedef usz uptr;

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

// fn void must(bool condition, ) noexcept;

// Copies n bytes of memory from src to dst
//
// Implementation is platform-specific and will likely wrap
// memory copy system call
fn void copy(u8* src, u8* dst, usz n) noexcept;

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
};

// shorthand for C string to memory chunk literal conversion
#define macro_static_str(s) mc((u8*)u8##s, sizeof(u8##s) - 1)

#define macro_src_file mc((u8*)__FILE__, sizeof(__FILE__) - 1)
#define macro_src_line cast(u32, __LINE__)

internal const u8 max_small_decimal = 99;
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
  if (x < 10) {
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

  method usz write(mc c) noexcept { return write(c.ptr, c.len); }

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

  method usz fmt_dec(u8 x) noexcept {
    var mc t = tail();
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_dec(u32 x) noexcept { return fmt_dec(cast(u64, x)); }

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

  // Returns Memory Chunk which is occupied by actual data
  // inside Bytes Buffer
  method mc head() noexcept { return mc(ptr, len); }

  // Returns Memory Chunk which is a portion of Bytes Buffer
  // that is available for writes
  method mc tail() noexcept { return mc(tip(), rem()); }
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

// Stores elements and their number together as a single data structure
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
  };

  mc data;

  Code code;

  let FileReadResult(mc d) noexcept : data(d), code(Code::Ok) {}
  let FileReadResult(Code c) noexcept : data(mc()), code(c) {}
  let FileReadResult(mc d, Code c) noexcept : data(d), code(c) {}

  method bool is_ok() noexcept { return code == Code::Ok; }
  method bool is_err() noexcept { return code != Code::Ok; }
};

#endif  // GUARD_CORE_HPP
