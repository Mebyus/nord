namespace coven::hash::djb2 {

internal const u64 magic = 5381;

fn u64 compute(mc c) noexcept {
  var u64 h = magic;
  for (uarch i = 0; i < c.len; i += 1) {
    h = ((h << 5) + h) + c.ptr[i];
  }
  return h;
}

}  // namespace coven::hash::djb2

namespace coven::hash::fnv64a {

internal const u64 offset = 14695981039346656037ULL;
internal const u64 prime = 1099511628211;

struct Hasher {
  u64 sum;

  let constexpr Hasher() noexcept : sum(offset) {}

  method void reset() noexcept { sum = offset; }

  method void write(mc c) noexcept {
    var u64 hash = sum;
    for (uarch i = 0; i < c.len; i += 1) {
      hash ^= cast(u64, c.ptr[i]);
      hash *= prime;
    }
    sum = hash;
  }

  method void write(u8 x) noexcept {
    var u64 hash = sum;
    hash ^= cast(u64, x);
    hash *= prime;
    sum = hash;
  }

};

fn u64 compute(mc c) noexcept {
    var Hasher h = Hasher();
    h.write(c);
    return h.sum;
}

} // namespace coven::hash::fnv64a

namespace coven::hash::map {

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

}  // namespace coven::hash::map
