#include "core.hpp"

namespace math {

fn f32 quake_reverse_square_root(f32 x) {
  const f32 h = 0.5f * x;
  var i32 i = bit_cast(i32, x);
  i = 0x5F3759DF - (i >> 1);
  x = bit_cast(f32, i);
  x = x * (1.5 - (h * x * x));
  return x;
}

fn f32 reverse_square_root(f32 x) {
  return 1.0f / __builtin_sqrt(x);
}

}  // namespace math
