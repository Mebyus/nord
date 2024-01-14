#include "core.hpp"

namespace math {

fn inline constexpr f32 square_root(f32 x) noexcept {
  return __builtin_sqrtf(x);
}

fn inline constexpr f32 quake_inverse_square_root(f32 x) noexcept {
  const f32 h = 0.5f * x;
  var i32 i = bit_cast(i32, x);
  i = 0x5F3759DF - (i >> 1);
  x = bit_cast(f32, i);
  x = x * (1.5f - (h * x * x));
  return x;
}

fn inline constexpr f32 inverse_square_root(f32 x) noexcept {
  return 1.0f / __builtin_sqrtf(x);
}

union vec3 {
  f32 p[3];
  struct {
    f32 x;
    f32 y;
    f32 z;
  };

  let constexpr vec3(f32 x, f32 y, f32 z) noexcept : x(x), y(y), z(z) {}
};

fn inline constexpr vec3 add(vec3 a, vec3 b) noexcept {
  return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

fn inline constexpr vec3 sub(vec3 a, vec3 b) noexcept {
  return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

fn inline constexpr vec3 mul(f32 k, vec3 a) noexcept {
  return vec3(k * a.x, k * a.y, k * a.z);
}

fn inline constexpr f32 dot(vec3 a, vec3 b) noexcept {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

fn inline constexpr vec3 cross(vec3 a, vec3 b) noexcept {
  const f32 x = a.y * b.z - a.z * b.y;
  const f32 y = a.z * b.x - a.x * b.z;
  const f32 z = a.x * b.y - a.y * b.x;

  return vec3(x, y, z);
}

fn inline constexpr f32 abs(vec3 a) noexcept {
  return square_root(dot(a, a));
}

struct mat3 {
  f32 xx, xy, xz;
  f32 yx, yy, yz;
  f32 zx, zy, zz;
};

fn inline constexpr vec3 mul(mat3 m, vec3 v) noexcept {
  const f32 x = m.xx * v.x + m.xy * v.y + m.xz * v.z;
  const f32 y = m.yx * v.x + m.yy * v.y + m.yz * v.z;
  const f32 z = m.zx * v.x + m.zy * v.y + m.zz * v.z;

  return vec3(x, y, z);
}

fn inline constexpr mat3 mul(mat3 a, mat3 b) noexcept {}

fn inline constexpr mat3 mul(f32 k, mat3 m) noexcept {}

// Unit quaternion (i.e. with length equal 1)
struct uqn {
  // real part
  f32 r;

  // vector part
  f32 x;
  f32 y;
  f32 z;

  let constexpr uqn(f32 r) noexcept : r(r), x(0.0), y(0.0), z(0.0) {}

  let constexpr uqn(vec3 v) noexcept : r(0.0), x(v.x), y(v.y), z(v.z) {}
`
  let constexpr uqn(f32 r, f32 x, f32 y, f32 z) noexcept
      : r(r), x(x), y(y), z(z) {}

  let constexpr uqn(f32 r, vec3 v) noexcept : r(r), x(v.x), y(v.y), z(v.z) {}

  method inline constexpr vec3 vec() noexcept { return vec3(x, y, z); }
};

fn inline constexpr uqn conj(uqn a) noexcept {
  return uqn(a.r, -a.x, -a.y, -a.z);
}

fn inline constexpr uqn mul(uqn a, uqn b) noexcept {
  const vec3 av = a.vec();
  const vec3 bv = b.vec();
  const f32 r = a.r * b.r - dot(av, bv);
  const vec3 v = add(add(mul(a.r, bv), mul(b.r, av)), cross(av, bv));

  return uqn(r, v);
}

}  // namespace math
