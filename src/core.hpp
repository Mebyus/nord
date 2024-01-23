// Small object that holds information about non-fatal error
struct error {
  // Unique identifier of error origin location in code base
  usz origin;

  //
  usz kind;

  method bool is_nil() noexcept { return kind == 0; }
};

namespace mem {

struct Gen {
  let Gen() noexcept {}

  method mc alloc(usz n) noexcept {
    must(n != 0);

    var u8* ptr = cast(u8*, malloc(n));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  // Allocate a non-overlapping copy of given memory chunk
  method mc allocate_copy(mc c) noexcept {
    var mc cp = alloc(c.len);
    cp.unsafe_write(c);
    return cp;
  }

  template <typename T>
  method chunk<T> alloc(usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(n != 0);

    var T* ptr = cast(T*, malloc(chunk_size(T, n)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method mc calloc(usz n) noexcept {
    must(n != 0);

    var u8* ptr = cast(u8*, ::calloc(n, 1));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  template <typename T>
  method chunk<T> calloc(usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(n != 0);

    var T* ptr = cast(T*, ::calloc(n, sizeof(T)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method mc realloc(mc c, usz n) noexcept {
    must(c.ptr != nil && c.len != 0 && n != 0 && c.len != n);

    var u8* ptr = cast(u8*, ::realloc(c.ptr, n));
    if (ptr == nil) {
      return mc();
    }
    return mc(ptr, n);
  }

  template <typename T>
  method chunk<T> realloc(chunk<T> c, usz n) noexcept {
    static_assert(sizeof(T) != 0);
    must(c.ptr != nil && c.len != 0 && n != 0 && c.len != n);

    var T* ptr = cast(T*, ::realloc(c.ptr, chunk_size(T, n)));
    if (ptr == nil) {
      return chunk<T>();
    }
    return chunk<T>(ptr, n);
  }

  method void free(mc c) noexcept {
    must(c.ptr != nil && c.len != 0);

    ::free(c.ptr);
  }

  template <typename T>
  method void free(chunk<T> c) noexcept {
    must(c.ptr != nil && c.len != 0);

    ::free(c.ptr);
  }
};

var global Gen gpa_global_instance = Gen();

fn mc alloc(usz n) noexcept {
  return gpa_global_instance.alloc(n);
}

template <typename T>
fn chunk<T> alloc(usz n) noexcept {
  return gpa_global_instance.alloc<T>(n);
}

fn mc calloc(usz n) noexcept {
  return gpa_global_instance.calloc(n);
}

template <typename T>
fn chunk<T> calloc(usz n) noexcept {
  return gpa_global_instance.calloc<T>(n);
}

fn mc realloc(mc c, usz n) noexcept {
  return gpa_global_instance.realloc(c, n);
}

template <typename T>
fn chunk<T> realloc(chunk<T> c, usz n) noexcept {
  return gpa_global_instance.realloc<T>(c, n);
}

fn void free(mc c) noexcept {
  gpa_global_instance.free(c);
}

template <typename T>
fn void free(chunk<T> c) noexcept {
  gpa_global_instance.free<T>(c);
}

struct alc {
  enum struct Kind : u8 {
    Nil,
    Arena,
    Gen,
  };

  anyptr ptr;

  Kind kind;

  let alc() noexcept : ptr(nil), kind(Kind::Nil) {}

  let alc(Arena* arena) noexcept
      : ptr(cast(anyptr, arena)), kind(Kind::Arena) {}

  let alc(Gen* gen) noexcept : ptr(cast(anyptr, gen)), kind(Kind::Gen) {}

  method bool is_nil() const noexcept { return kind == Kind::Nil; }

  method mc alloc(usz n) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        return gen->alloc(n);
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }

  method mc realloc(mc c, usz n) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        return gen->realloc(c, n);
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }

  method void free(mc c) noexcept {
    switch (kind) {
      case Kind::Nil: {
        panic(macro_static_str("nil allocator"));
      }

      case Kind::Arena: {
        noimp();
      }

      case Kind::Gen: {
        var Gen* gen = cast(Gen*, ptr);
        gen->free(c);
        return;
      }

      default: {
        unreachable();
      }
    }

    unreachable();
  }
};

}  // namespace mem

fn internal inline usz determine_bytes_grow_amount(usz cap, usz len) noexcept {
  if (len <= cap) {
    const usz size_four_megabytes = 1 << 22;
    if (cap < size_four_megabytes) {
      return cap;
    }

    return len + (cap >> 10);
  }

  if (cap != 0) {
    return len + (cap >> 1);
  }

  if (len < 16) {
    return 16;
  }

  if (len < 64) {
    return 64;
  }

  return len + (len >> 1);
}

// Dynamically allocated bytes buffer. Default global general
// purpose allocator is used to manage underlying memory
struct DynBytesBuffer {
  bb buf;

  let DynBytesBuffer() noexcept : buf(bb()) {}

  let DynBytesBuffer(usz n) noexcept : buf(bb()) { init(n); }

  // Allocates a dynamic copy of a given memory chunk
  let DynBytesBuffer(mc c) noexcept : buf(bb()) {
    if (c.is_nil()) {
      return;
    }
    init(c.len);
    buf.unsafe_write(c);
  }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = bb(mem::alloc(n));
  }

  method bool is_nil() const noexcept { return buf.is_nil(); }

  // Increase buffer capacity by at least n bytes
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = bb(mem::alloc(n));
      return;
    }

    const usz len = buf.len;

    buf = bb(mem::realloc(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void write(mc c) noexcept {
    if (c.is_nil()) {
      return;
    }

    if (buf.rem() >= c.len) {
      buf.unsafe_write(c);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, c.len);
    grow(n);

    buf.unsafe_write(c);
  }

  // Insert single byte at specified index. All bytes with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  method void insert(usz i, u8 x) noexcept {
    must(i <= buf.len);

    if (buf.rem() >= 1) {
      buf.unsafe_insert(i, x);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_insert(i, x);
  }

  // Remove single byte at specified index. All active bytes with
  // greater indices will be shifted by one position to the left.
  // This operation decreases buffer length by 1
  method void remove(usz i) noexcept {
    must(i < buf.len);

    buf.unsafe_remove(i);
  }

  method void write(u8 x) noexcept {
    if (buf.rem() >= 1) {
      buf.unsafe_write(x);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_write(x);
  }

  method void write_repeat(usz n, u8 x) noexcept {
    if (buf.rem() >= n) {
      buf.unsafe_write_repeat(n, x);
      return;
    }

    const usz amount = determine_bytes_grow_amount(buf.cap, n);
    grow(amount);

    buf.unsafe_write_repeat(n, x);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    mem::free(buf.body());
  }

  method void reset() noexcept { buf.reset(); }

  method void reset(usz n) noexcept {
    must(n <= buf.len);

    buf.len = n;
  }

  method mc head() const noexcept { return buf.head(); }
};

// Dynamically allocated bytes buffer
struct dbb {
  bb buf;

  // Allocator used to pull/release memory for buffer
  mem::alc alc;

  let dbb() noexcept : buf(bb()), alc(mem::alc()) {}

  let dbb(mem::alc a) noexcept : buf(bb()), alc(a) {}

  let dbb(mem::alc a, usz n) noexcept : buf(bb()), alc(a) { init(n); }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = bb(alc.alloc(n));
  }

  method bool is_nil() const noexcept { return alc.is_nil(); }

  // Increase buffer capacity by at least n bytes
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = bb(alc.alloc(n));
      return;
    }

    const usz len = buf.len;

    buf = bb(alc.realloc(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void write(mc c) noexcept {
    if (c.is_nil()) {
      return;
    }

    if (buf.rem() >= c.len) {
      buf.unsafe_write(c);
      return;
    }

    // calculate by how much we need to grow buffer capacity
    var dirty usz n;
    if (c.len <= buf.cap) {
      n = buf.cap;
    } else if (buf.cap != 0) {
      n = c.len + (buf.cap >> 1);
    } else {
      if (c.len < 16) {
        n = 16;
      } else if (c.len < 64) {
        n = 64;
      } else {
        n = c.len + (c.len >> 1);
      }
    }

    grow(n);

    buf.unsafe_write(c);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    alc.free(buf.body());
  }
};

fn internal inline usz determine_grow_amount(usz cap, usz len) noexcept {
  if (len <= cap) {
    const usz size_threshold = 1 << 16;
    if (cap < size_threshold) {
      return cap;
    }

    return len + (cap >> 8);
  }

  if (cap != 0) {
    return len + (cap >> 1);
  }

  if (len < 16) {
    return 16;
  }

  if (len < 64) {
    return 64;
  }

  return len + (len >> 1);
}

// Dynamically allocated buffer of elements (of the same type).
// It is most similar to dynamic arrays from other libraries or
// languages. Default global general purpose allocator is used
// to manage underlying memory
template <typename T>
struct DynBuffer {
  buffer<T> buf;

  let DynBuffer() noexcept : buf(buffer<T>()) {}

  let DynBuffer(usz n) noexcept : buf(buffer<T>()) { init(n); }

  method void init(usz n) noexcept {
    if (n == 0) {
      return;
    }
    buf = buffer<T>(mem::alloc<T>(n));
  }

  // Increase buffer capacity by at least n elements
  method void grow(usz n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = buffer<T>(mem::alloc<T>(n));
      return;
    }

    const usz len = buf.len;

    buf = buffer<T>(mem::realloc<T>(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void append(T elem) noexcept {
    if (buf.rem() >= 1) {
      buf.unsafe_append(elem);
      return;
    }

    const usz n = determine_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_append(elem);
  }

  // Insert single element at specified index. All elements with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  method void insert(usz i, T x) noexcept {
    must(i <= buf.len);

    if (buf.rem() >= 1) {
      buf.unsafe_insert(i, x);
      return;
    }

    const usz n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_insert(i, x);
  }

  // Remove single element at specified index. All active elements with
  // greater indices will be shifted by one position to the left.
  // This operation decreases buffer length by 1
  method void remove(usz i) noexcept {
    must(i < buf.len);

    buf.unsafe_remove(i);
  }

  method void free() noexcept {
    if (buf.is_nil()) {
      return;
    }
    mem::free(buf.body());
  }

  method void reset() noexcept { buf.reset(); }

  method usz len() const noexcept { return buf.len; }

  method chunk<T> head() const noexcept { return buf.head(); }
};

// Copies entire memory chunk from source into destination and
// adds null terminator at the the end. Resulting C string in
// destination memory chunk will occupy exactly source length + 1
// bytes. Destination must have enough space to hold that much
// data
//
// Returns C string structure which borrows memory from destination
fn cstr unsafe_copy_as_cstr(str src, mc dst) noexcept {
  must(dst.len >= src.len + 1);

  dst.unsafe_write(src);
  dst.ptr[src.len] = 0;
  return cstr(dst.ptr, src.len);
}

#define macro_static_cstr(s) cstr((u8*)u8##s, sizeof(u8##s) - 1)





namespace debug {

// caller should supply memory chunk of at least 256 bytes long
fn mc print_types_info(mc c) noexcept {
  var bb buf = bb(c);

  buf.write(macro_static_str("\nu8\t\t"));
  buf.fmt_dec(sizeof(u8));

  buf.write(macro_static_str("\nu16\t\t"));
  buf.fmt_dec(sizeof(u16));

  buf.write(macro_static_str("\nu32\t\t"));
  buf.fmt_dec(sizeof(u32));

  buf.write(macro_static_str("\nu64\t\t"));
  buf.fmt_dec(sizeof(u64));

  buf.write(macro_static_str("\nu128\t\t"));
  buf.fmt_dec(sizeof(u128));

  buf.lf();

  buf.write(macro_static_str("\ni8\t\t"));
  buf.fmt_dec(sizeof(i8));

  buf.write(macro_static_str("\ni16\t\t"));
  buf.fmt_dec(sizeof(i16));

  buf.write(macro_static_str("\ni32\t\t"));
  buf.fmt_dec(sizeof(i32));

  buf.write(macro_static_str("\ni64\t\t"));
  buf.fmt_dec(sizeof(i64));

  buf.write(macro_static_str("\ni128\t\t"));
  buf.fmt_dec(sizeof(i128));

  buf.lf();

  buf.write(macro_static_str("\nf32\t\t"));
  buf.fmt_dec(sizeof(f32));

  buf.write(macro_static_str("\nf64\t\t"));
  buf.fmt_dec(sizeof(f64));

  buf.write(macro_static_str("\nf128\t\t"));
  buf.fmt_dec(sizeof(f128));

  buf.lf();

  buf.write(macro_static_str("\nusz\t\t"));
  buf.fmt_dec(sizeof(usz));

  buf.write(macro_static_str("\nisz\t\t"));
  buf.fmt_dec(sizeof(isz));

  buf.write(macro_static_str("\nuptr\t\t"));
  buf.fmt_dec(sizeof(void*));

  buf.lf();

  buf.write(macro_static_str("\nmc\t\t"));
  buf.fmt_dec(sizeof(mc));

  buf.write(macro_static_str("\nbb\t\t"));
  buf.fmt_dec(sizeof(bb));

  buf.lf();

  return buf.head();
}

}  // namespace debug



