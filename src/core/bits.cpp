namespace coven::bits {

internal const u16 max_u16 = ~cast(u16, 0);
internal const u32 max_u32 = ~cast(u32, 0);
internal const u64 max_u64 = ~cast(u64, 0);

fn internal inline constexpr u32 upper_power_of_two(u32 x) noexcept {
  x -= 1;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x += 1;
  return x;
}

fn internal inline constexpr u32 trailing_zeros(u32 x) noexcept {
  if (x == 0) {
    return 32;
  }

  return __builtin_ctz(x);
}

fn internal inline constexpr u64 trailing_zeros(u64 x) noexcept {
  if (x == 0) {
    return 64;
  }

  return __builtin_ctzl(x);
}

fn internal inline constexpr bool is_power_of_2(u32 x) noexcept {
  return (x & (x - 1)) == 0;
}

// Returns true if given integer can be represented as x = 2 ** n
// (power of 2), where n is a non-negative integer n >= 0
//
// As a special case returns true for x = 0
fn internal inline constexpr bool is_power_of_2(u64 x) noexcept {
  return (x & (x - 1)) == 0;
}

fn internal inline constexpr bool is_aligned_by_16(void* x) noexcept {
  return (cast(uptr, x) & 0x0F) == 0;
}

fn internal inline constexpr bool is_aligned_by_16(uarch x) noexcept {
  return (x & 0x0F) == 0;
}

fn internal inline constexpr uarch align_by_16(uarch x) noexcept {
  var uarch a = x & 0x0F;
  a = ((~a) + 1) & 0x0F;
  return x + a;
}

fn internal inline constexpr uarch align_by_4kb(uarch x) noexcept {
  var uarch a = x & 0x3FF;
  a = ((~a) + 1) & 0x3FF;
  return x + a;
}

}  // namespace coven::bits