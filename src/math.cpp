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

  let constexpr vec3() noexcept : x(0.0f), y(0.0f), z(0.0f) {}

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

// Return normalized (unit) vector from a given one. Unit vector
// has the same direction and length of exactly 1
fn inline constexpr vec3 norm(vec3 a) noexcept {
  return mul(inverse_square_root(dot(a, a)), a);
}

// Matrix 3 x 3. First index traverses rows, second - columns.
// In table form matrix looks as follows:
//
//  | xx  xy  xz |
//  |            |
//  | yx  yy  yz |
//  |            |
//  | zx  zy  zz |
//
struct mat3 {
  f32 xx, xy, xz;
  f32 yx, yy, yz;
  f32 zx, zy, zz;

  // Create a diagonal matrix
  //
  // All diagonal elements are equal to a given number.
  // All non-diagonal elemetns are set to zero
  let constexpr mat3(f32 k) noexcept
      : xx(k),
        xy(0.0f),
        xz(0.0f),
        //
        yx(0.0f),
        yy(k),
        yz(0.0f),
        //
        zx(0.0f),
        zy(0.0f),
        zz(k)  //
  {}

  // Create a diagonal matrix
  //
  // All diagonal elements are equal to a given corresponding
  // vector coordinate. All non-diagonal elemetns are set to zero
  let constexpr mat3(vec3 a) noexcept
      : xx(a.x),
        xy(0.0f),
        xz(0.0f),
        //
        yx(0.0f),
        yy(a.y),
        yz(0.0f),
        //
        zx(0.0f),
        zy(0.0f),
        zz(a.z)  //
  {}

  let constexpr mat3(vec3 x, vec3 y, vec3 z) noexcept
      : xx(x.x),
        xy(y.x),
        xz(z.x),
        //
        yx(x.y),
        yy(y.y),
        yz(z.y),
        //
        zx(x.z),
        zy(y.z),
        zz(z.z)  //
  {}

  method inline constexpr vec3 col_x() noexcept { return vec3(xx, yx, zx); }

  method inline constexpr vec3 col_y() noexcept { return vec3(xy, yy, zy); }

  method inline constexpr vec3 col_z() noexcept { return vec3(xz, yz, zz); }

  method inline constexpr vec3 row_x() noexcept { return vec3(xx, xy, xz); }

  method inline constexpr vec3 row_y() noexcept { return vec3(yx, yy, yz); }

  method inline constexpr vec3 row_z() noexcept { return vec3(zx, zy, zz); }
};

// Create one dimensional projection matrix onto a given vector
//
// Applying this matrix to another vector gives result equal to
// mul(dot(v, a), v)
fn inline constexpr mat3 proj1(vec3 v) noexcept {
  const vec3 x = vec3(v.x * v.x, v.y * v.x, v.z * v.x);
  const vec3 y = vec3(v.x * v.y, v.y * v.y, v.z * v.y);
  const vec3 z = vec3(v.x * v.z, v.y * v.z, v.z * v.z);

  return mat3(x, y, z);
}

// Create a matrix with transformation corresponding to cross
// product of a fixed vector with a vector being transformed
//
// Function accepts a fixed vector. Result of applying the matrix
// to another vector can be described as:
//
//  mul(m, a) = cross(v, a)
fn inline constexpr mat3 cross(vec3 v) noexcept {

}

fn inline constexpr mat3 add(mat3 a, mat3 b) noexcept {
  const vec3 x = mul(a.col_x(), b.col_x());
  const vec3 y = mul(a.col_y(), b.col_y());
  const vec3 z = mul(a.col_z(), b.col_z());

  return mat3(x, y, z);
}

fn inline constexpr mat3 mul(f32 k, mat3 m) noexcept {
  const vec3 x = mul(k, m.col_x());
  const vec3 y = mul(k, m.col_y());
  const vec3 z = mul(k, m.col_z());

  return mat3(x, y, z);
}

fn inline constexpr vec3 mul(mat3 m, vec3 v) noexcept {
  const f32 x = m.xx * v.x + m.xy * v.y + m.xz * v.z;
  const f32 y = m.yx * v.x + m.yy * v.y + m.yz * v.z;
  const f32 z = m.zx * v.x + m.zy * v.y + m.zz * v.z;

  return vec3(x, y, z);
}

fn inline constexpr mat3 mul(mat3 a, mat3 b) noexcept {
  const vec3 ax = a.row_x();
  const vec3 ay = a.row_y();
  const vec3 az = a.row_z();

  const vec3 bx = a.col_x();
  const vec3 by = a.col_y();
  const vec3 bz = a.col_z();

  const vec3 x = vec3(dot(ax, bx), dot(ay, bx), dot(az, bx));
  const vec3 y = vec3(dot(ax, by), dot(ay, by), dot(az, by));
  const vec3 z = vec3(dot(ax, bz), dot(ay, bz), dot(az, bz));

  return mat3(x, y, z);
}

fn inline constexpr mat3 mul(f32 k, mat3 m) noexcept {
  const vec3 x = mul(k, vec3(m.xx, m.yx, m.zx));
  const vec3 y = mul(k, vec3(m.xy, m.yy, m.zy));
  const vec3 z = mul(k, vec3(m.xz, m.yz, m.zz));

  return mat3(x, y, z);
}

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

  let constexpr uqn(f32 r, f32 x, f32 y, f32 z) noexcept
      : r(r), x(x), y(y), z(z) {}

  let constexpr uqn(f32 r, vec3 v) noexcept : r(r), x(v.x), y(v.y), z(v.z) {}

  method inline constexpr vec3 vec() noexcept { return vec3(x, y, z); }

  // Produce rotation matrix based on this unit quaternion
  method inline constexpr mat3 rot() noexcept {
    const f32 k = 2.0f * r * r - 1.0;
    const f32 p = 2.0f * r;
    const vec3 v = vec();

    return add(mat3(k), mul(2.0f, proj1(v)));
  }
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

// Special float (32-bit) 3D operations
namespace s3d {
// Name stands for "matrix extended". Represents affine transformation
// in 3D vector space. It does so by combining a regular matrix 3 x 3
// and translation vector 3 x 1 into a single matrix 4 x 4. Resulting
// layout looks as follows:
//
//  | xx  xy  xz  tx |
//  |                |
//  | yx  yy  yz  ty |
//  |                |
//  | zx  zy  zz  tz |
//  |                |
//  |  0   0   0   1 |
//
// Elements denoted by tj are components of translation vector
//
// Matrix 4 x 4. First index traverses rows, second - columns.
// In table form matrix looks as follows:
//
//  | xx  xy  xz  xw |
//  |                |
//  | yx  yy  yz  yw |
//  |                |
//  | zx  zy  zz  zw |
//  |                |
//  | wx  wy  wz  ww | <-- this row is always fixed to |  0  0  0  1  |
struct mex {
  f32 xx, xy, xz, xw;
  f32 yx, yy, yz, yw;
  f32 zx, zy, zz, zw;
  f32 wx, wy, wz, ww;
};

}  // namespace s3d
