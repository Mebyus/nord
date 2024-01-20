// Use only for comparison with pointer types
#define nil 0

#undef min
#undef max
#undef EOF
#undef stdin
#undef stdout
#undef stderr

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
