namespace coven::os {

struct AllocResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    NoMemoryAvailable,

    TooManyMappings,
  };

  mc m;

  Code code;

  let AllocResult(mc m) noexcept : m(m), code(Code::Ok) {}
  let AllocResult(Code code) noexcept : m(mc()), code(code) {}
};

// Allocates (maps) at least n bytes of virtual memory in whole pages
//
// Length of returned memory chunk is always a multiple of
// memory page size and not smaller than argument n. Memory
// is requested directly from OS, that is each call to this
// function will translate to system call without any internal
// buffering
//
// Returns nil chunk if OS returns an error while allocating
// a requested amount of memory
fn AllocResult alloc(uarch n) noexcept;

struct FreeResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,
  };

  Code code;
};

// Free (unmap) memory chunk that was received from alloc
// function
fn FreeResult free(mc c) noexcept;

} // namespace coven::os

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

fn void move(u8* restrict src, u8* dst, uarch n) noexcept {
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

  template <typename T>
  method chunk<T> alloc(uarch n) noexcept {
    static_assert(sizeof(T) != 0);

    const mc c = alloc(chunk_size(T, n));

    return chunk<T>(cast(T*, c.ptr), c.len / sizeof(T));
  }

  method mc calloc(uarch n) noexcept {
    var mc c = alloc(n);
    c.clear();
    return c;
  }

  template <typename T>
  method chunk<T> calloc(uarch n) noexcept {
    static_assert(sizeof(T) != 0);

    const mc c = calloc(chunk_size(T, n));

    return chunk<T>(cast(T*, c.ptr), c.len / sizeof(T));
  }

  method mc realloc(mc c, uarch n) noexcept {
    must(n > c.len);

    var mc c2 = alloc(n);
    c2.unsafe_write(c);
    return c2;
  }

  template <typename T>
  method chunk<T> realloc(chunk<T> c, uarch n) noexcept {
    static_assert(sizeof(T) != 0);

    const mc c2 = realloc(c.as_mc(), chunk_size(T, n));

    return chunk<T>(cast(T*, c2.ptr), c2.len / sizeof(T));
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

  method uarch rem() const noexcept { return buf.len - pos; }

  // Pop n bytes from previous allocations, marking them as
  // available for next allocations. Popped memory will no
  // longer be valid for clients who aliased it
  //
  // Use this operation with extreme caution. It is primary usage
  // intended for deallocating short-lived scratch buffers which
  // lifetime is limited by narrow scope
  method void pop(uarch n) noexcept {
    must(n != 0);
    must(bits::is_aligned_by_16(n));
    must(n <= pos);

    pos -= n;
  }

  // Drops all allocated memory and makes entire buffer
  // available for future allocations
  method void reset() noexcept { pos = 0; }
};

internal const uarch global_arena_size = 1 << 26;
var global Arena arena = Arena(os::alloc(global_arena_size).m); // TODO: handle alloc error

fn mc alloc(uarch n) noexcept {
  return arena.alloc(n);
}

template <typename T>
fn chunk<T> alloc(uarch n) noexcept {
  return arena.alloc<T>(n);
}

fn mc calloc(uarch n) noexcept {
  return arena.calloc(n);
}

template <typename T>
fn chunk<T> calloc(uarch n) noexcept {
  return arena.calloc<T>(n);
}

fn mc realloc(mc c, uarch n) noexcept {
  return arena.realloc(c, n);
}

template <typename T>
fn chunk<T> realloc(chunk<T> c, uarch n) noexcept {
  return arena.realloc<T>(c, n);
}

fn void free(mc c) noexcept {
  dummy_usage(c);
  // arena.free(c);
}

template <typename T>
fn void free(chunk<T> c) noexcept {
  dummy_usage(c);
  // arena.free<T>(c);
}

} // namespace coven::mem
