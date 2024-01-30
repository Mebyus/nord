// Use only for comparison with pointer types
#define nil 0

// Modify function or constant to be bound to translation unit
#define internal static

// Modify global variable to be bound to translation unit
#define global static
#define persist static

#define restrict __restrict__

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

// Dummy usage for variable, argument or constant
//
// Suppresses compiler warnings
#define dummy_usage(x) (void)(x)

// Macro for unambui type cast
#define cast(T, x) (T)(x)

#define bit_cast(T, x) __builtin_bit_cast(T, x)

// Compute chunk size in bytes to hold n elements of type T
#define chunk_size(T, n) (n * sizeof(T))

namespace coven {

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long int u64;
typedef __uint128_t u128;

typedef signed char i8;
typedef signed short int i16;
typedef signed int i32;
typedef signed long int i64;
typedef __int128_t i128;

typedef float f32;
typedef double f64;
typedef __float128 f128;

// Platform architecture dependent unsigned integer type
//
// Always has enough bytes to represent size of any memory
// region (or offset in memory region)
typedef u64 uarch;

// Platform architecture dependent signed integer type
//
// Always has the same size as ua type
typedef i64 iarch;

// Type for performing arbitrary arithmetic pointer operations
// or comparisons
typedef uarch uptr;

// Rune represents a unicode code point
typedef u32 rune;

template <typename T>
fn internal inline constexpr T min(T a, T b) noexcept {
  if (a < b) {
    return a;
  }
  return b;
}

template <typename T>
fn internal inline constexpr T max(T a, T b) noexcept {
  if (a < b) {
    return b;
  }
  return a;
}

template <typename T>
fn internal inline void swap(T& a, T& b) noexcept {
  T x = a;
  a = b;
  b = x;
}

fn [[noreturn]] internal inline void unreachable() noexcept {
  __builtin_unreachable();
}

fn [[noreturn]] internal inline void crash() noexcept {
  // Trap outputs special reserved instruction in machine code.
  // Upon executing this instruction a cpu exception is generated
  __builtin_trap();
  unreachable();
}

// Basic assert function. If condition is not met crashes the program
fn internal inline void must(bool condition) noexcept {
  if (condition) {
    return;
  }

  crash();
}

} // namespace coven

namespace coven::mem {

// Copies n bytes of memory from src to dst. Number of copied bytes
// must be greater than zero
//
// Do not use for overlapping memory regions. 
fn void copy(u8* restrict src, u8* restrict dst, uarch n) noexcept;

// Copies n bytes of memory from src to dst. Number of copied bytes
// must be greater than zero
//
// Guarantees correct behaviour for overlapping memory regions
fn void move(u8* restrict src, u8* dst, iarch n) noexcept;

} // namespace coven::mem
