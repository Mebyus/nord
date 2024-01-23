namespace coven::mem {

fn void copy(u8* restrict src, u8* restrict dst, uarch n) noexcept {
  must(n != 0);
  must(src != 0);
  must(dst != 0);
  must(src != dst);

  for (uarch i = 0; i < n; i += 1) {
    dst[i] = src[i];
  }
}

fn void move(u8* restrict src, u8* dst, iarch n) noexcept {
  must(n != 0);
  must(src != 0);
  must(dst != 0);
  must(src != dst);

  const uptr a = cast(uptr, src);
  const uptr b = cast(uptr, dst);

  if (a > b) {
    must(a - b < n); // if violated than regular copy should be used

    for (uarch i = 0; i < n; i += 1) {
      dst[i] = src[i];
    }
    return;
  }

  must(b - a < n); // if violated than regular copy should be used

  // copy starting from the end
  var uarch i = n;
  do {
    i -= 1;
    dst[i] = src[i];
  } while (i != 0);
}

struct Arena {
  // Internal buffer which is used for allocating chunks
  mc buf;

  // Position of next chunk to be allocated. Can also be
  // interpreted as total amount of bytes already used
  // inside arena
  uarch pos;

  let Arena(mc buf) noexcept : buf(buf), pos(0) {
    must(!buf.is_nil());
    must(bits::is_aligned_by_16(buf.ptr));
  }

  // Allocate at least n bytes of memory
  //
  // Returns allocated memory chunk. Note that its length
  // may be bigger than number of bytes requested
  method mc alloc(uarch n) noexcept {
    must(n != 0);

    n = bits::align_by_16(n);
    must(n <= rem());

    const uarch prev = pos;
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

  method usz rem() const noexcept { return buf.len - pos; }

  // Pop n bytes from previous allocations, marking them as
  // available for next allocations. Popped memory will no
  // longer be valid for clients who aliased it
  //
  // Use this operation with extreme caution. It is primary usage
  // intended for deallocating short-lived scratch buffers which
  // lifetime is limited by narrow scope
  method void pop(usz n) noexcept {
    must(n != 0);
    must(bits::is_aligned_by_16(n));
    must(n <= pos);

    pos -= n;
  }

  // Drops all allocated memory and makes entire buffer
  // available for future allocations
  method void reset() noexcept { pos = 0; }
};

} // namespace coven::mem
