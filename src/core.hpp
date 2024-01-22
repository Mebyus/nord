// Small object that holds information about non-fatal error
struct error {
  // Unique identifier of error origin location in code base
  usz origin;

  //
  usz kind;

  method bool is_nil() noexcept { return kind == 0; }
};

// Bytes Buffer
//
// Accumulates multiple writes into single continuous memory region.
// Buffer has capacity i.e. maximum number of bytes it can hold. After
// number of written bytes reaches this maximum all subsequent writes
// will write nothing and always return 0, until reset() is called
//
// Buffer is much alike memory chunk in terms of memory ownership
struct bb {
  u8* ptr;

  // number of bytes currently stored in buffer
  usz len;

  // buffer capacity i.e. maximum number of bytes it can hold
  usz cap;

  let constexpr bb() noexcept : ptr(nil), len(0), cap(0) {}

  let bb(mc c) noexcept : ptr(c.ptr), len(0), cap(c.len) {}

  let bb(u8* bytes, usz n) noexcept : ptr(bytes), len(0), cap(n) {}

  method bool is_empty() const noexcept { return len == 0; }

  method bool is_full() const noexcept { return len == cap; }

  method bool is_nil() const noexcept { return cap == 0; }

  // Returns number of bytes which can be written to buffer before it is full
  method usz rem() const noexcept { return cap - len; }

  method void reset() noexcept { len = 0; }

  // Returns pointer to first byte at buffer position
  method u8* tip() const noexcept { return ptr + len; }

  method usz write(u8* bytes, usz n) noexcept {
    var usz w = min(n, rem());
    if (w == 0) {
      return 0;
    }
    copy(bytes, tip(), w);
    len += w;
    return w;
  }

  method usz write(u8 b) noexcept {
    if (is_full()) {
      return 0;
    }
    ptr[len] = b;
    len += 1;
    return 1;
  }

  // add line feed (aka "newline") character to buffer
  method usz lf() noexcept { return write('\n'); }

  method usz write(mc c) noexcept { return write(c.ptr, c.len); }

  // Write specified byte repeated n times to memory chunk
  method usz write_repeat(usz n, u8 x) noexcept {
    var mc t = tail();
    const usz k = t.write_repeat(n, x);
    len += k;
    return k;
  }

  method void unsafe_write_repeat(usz n, u8 x) noexcept {
    var mc t = tail();
    t.unsafe_write_repeat(n, x);
    len += n;
  }

  method usz unsafe_write(mc c) noexcept {
    copy(c.ptr, tip(), c.len);
    len += c.len;
    return c.len;
  }

  // Use for writing character literals like this
  //
  // buf.unsafe_write('L');
  method void unsafe_write(char x) noexcept { unsafe_write(cast(u8, x)); }

  method void unsafe_write(u8 x) noexcept {
    ptr[len] = x;
    len += sizeof(u8);
  }

  method void unsafe_write(i16 x) noexcept {
    var i16* p = cast(i16*, tip());
    *p = x;
    len += sizeof(i16);
  }

  method void unsafe_write(f32 x) noexcept {
    var f32* p = cast(f32*, tip());
    *p = x;
    len += sizeof(f32);
  }

  // Insert single byte at specified index. All active bytes with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  //
  // For correct behaviour the following inequation must be
  // satisfied before the call:
  //
  // i <= len < cap
  //
  // In the special case of i == len new byte will be appended
  // at the tip of buffer
  //
  // This method does not perform any safety checks
  method void unsafe_insert(usz i, u8 x) noexcept {
    if (i == len) {
      unsafe_write(x);
      return;
    }

    move(ptr + i, ptr + i + 1, len - i);
    len += 1;
    ptr[i] = x;
  }

  // Remove single byte at specified index. All active bytes with
  // greater indices will be shifted by one position to the left.
  // This operation decreases buffer length by 1
  //
  // For correct behaviour the following inequation must be
  // satisfied before the call:
  //
  // i < len <= cap
  // (hence length must be at least 1)
  //
  // In the special case of i == len - 1 single byte will be popped
  // from the tip of buffer
  //
  // This method does not perform any safety checks
  method void unsafe_remove(usz i) noexcept {
    if (i == len - 1) {
      len -= 1;
      return;
    }

    move(ptr + i + 1, ptr + i, len - i - 1);
    len -= 1;
  }

  method usz fmt_dec(u8 x) noexcept {
    var mc t = tail();
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  // overload for convenient usage with sizeof() results
  method usz fmt_dec(unsigned long int x) noexcept {
    return fmt_dec(cast(u64, x));
  }

  method usz fmt_dec(u16 x) noexcept { return fmt_dec(cast(u64, x)); }

  method usz fmt_dec(u32 x) noexcept { return fmt_dec(cast(u64, x)); }

  method usz unsafe_fmt_dec(u32 x) noexcept {
    return unsafe_fmt_dec(cast(u64, x));
  }

  method usz unsafe_fmt_dec(u64 x) noexcept {
    var mc t = tail();
    var usz w = t.unsafe_fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_dec(u64 x) noexcept {
    if (is_full()) {
      return 0;
    }

    var mc t = tail();
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_dec(i64 x) noexcept {
    var mc t = tail();
    if (t.is_nil()) {
      return 0;
    }
    var usz w = t.fmt_dec(x);
    len += w;
    return w;
  }

  method usz fmt_bin_delim(u32 x) noexcept {
    var mc t = tail();
    var usz w = t.fmt_bin_delim(x);
    len += w;
    return w;
  }

  // Returns memory chunk which is occupied by actual data
  // inside buffer
  method mc head() const noexcept { return mc(ptr, len); }

  // Returns memory chunk which is a portion of buffer
  // that is available for writes
  method mc tail() const noexcept { return mc(tip(), rem()); }

  // Returns full body (from zero to capacity) of buffer
  method mc body() const noexcept { return mc(ptr, cap); }
};

// Buffer for storing elements (of the same type) in continuous memory region
// which allows appending new elements to the end of buffer until buffer
// capacity is reached
template <typename T>
struct buffer {
  // Pointer to buffer starting position
  T* ptr;

  // Number of elements stored in buffer
  usz len;

  // Buffer capacity i.e. maximum number of elements it can hold
  usz cap;

  let constexpr buffer() noexcept : ptr(nil), len(0), cap(0) {}

  let buffer(chunk<T> c) noexcept : ptr(c.ptr), len(0), cap(c.len) {}

  let buffer(T* p, usz n) noexcept : ptr(p), len(0), cap(n) {}

  method bool is_empty() const noexcept { return len == 0; }

  method bool is_full() const noexcept { return len == cap; }

  method bool is_nil() const noexcept { return cap == 0; }

  // Returns number of elements which can be appended to buffer before it is
  // full
  method usz rem() const noexcept { return cap - len; }

  method void reset() noexcept { len = 0; }

  // Returns chunk which is occupied by actual data
  // inside buffer
  method chunk<T> head() const noexcept { return chunk<T>(ptr, len); }

  // Append new element to buffer tail
  method usz append(T elem) noexcept {
    if (is_full()) {
      return 0;
    }

    unsafe_append(elem);
    return 1;
  }

  method void unsafe_append(T elem) noexcept {
    ptr[len] = elem;
    len += 1;
  }

  // Insert single element at specified index. All active elements with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  //
  // For correct behaviour the following inequation must be
  // satisfied before the call:
  //
  // i <= len < cap
  //
  // In the special case of i == len new element will be appended
  // at the tip of buffer
  //
  // This method does not perform any safety checks
  method void unsafe_insert(usz i, T x) noexcept {
    if (i == len) {
      unsafe_append(x);
      return;
    }

    move(cast(u8*, ptr + i), cast(u8*, ptr + i + 1), (len - i) * sizeof(T));
    len += 1;
    ptr[i] = x;
  }

  // Remove single element at specified index. All active elements with
  // greater indices will be shifted by one position to the left.
  // This operation decreases buffer length by 1
  //
  // For correct behaviour the following inequation must be
  // satisfied before the call:
  //
  // i < len <= cap
  // (hence length must be at least 1)
  //
  // In the special case of i == len - 1 single element will be popped
  // from the tip of buffer
  //
  // This method does not perform any safety checks
  method void unsafe_remove(usz i) noexcept {
    if (i == len - 1) {
      len -= 1;
      return;
    }

    move(cast(u8*, ptr + i + 1), cast(u8*, ptr + i), (len - i - 1) * sizeof(T));
    len -= 1;
  }

  method chunk<T> body() const noexcept { return chunk<T>(ptr, cap); }
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

// C String
//
// Represents a sequence of bytes in which the last byte always equals
// zero. Designed for interfacing with traditional C-style strings
struct cstr {
  // Pointer to first byte in string
  u8* ptr;

  // Number of bytes in string before zero terminator byte
  //
  // Thus actual number of stored bytes available through
  // pointer is len + 1
  usz len;

  let cstr() noexcept : ptr(nil), len(0) {}

  // This constructor is deliberately unsafe. Use it only
  // on C strings which come from trusted source
  //
  // For safe version with proper error-checking refer to
  // function try_parse_cstr
  let cstr(u8* s) noexcept {
    var usz i = 0;
    while (s[i] != 0) {
      i += 1;
    }
    ptr = s;
    len = i;
  }

  // Use this constructor if length of C string is already
  // known before the call. Second argument is number of
  // bytes in string, not including zero terminator
  let cstr(u8* s, usz n) noexcept : ptr(s), len(n) {}

  method bool is_nil() const noexcept { return len == 0; }

  // Returns Memory Chunk with stored bytes. Zero terminator
  // is not included
  method str as_str() const noexcept { return str(ptr, len); }

  // Returns Memory Chunk with stored bytes. Includes zero terminator
  // in its length
  method mc with_term() const noexcept { return mc(ptr, len + 1); }
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

namespace fs {

typedef usz FileDescriptor;

struct OpenResult {
  enum struct Code : u8 {
    Ok,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  FileDescriptor fd;

  Code code;

  let OpenResult(FileDescriptor f) noexcept : fd(f), code(Code::Ok) {}
  let OpenResult(Code c) noexcept : fd(0), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn OpenResult create(str filename) noexcept;

struct CloseResult {
  enum struct Code : u8 {
    Ok,

    // Generic error, no specifics known
    Error,
  };

  Code code;

  let CloseResult() noexcept : code(Code::Ok) {}
  let CloseResult(Code c) noexcept : code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn CloseResult close(FileDescriptor fd) noexcept;

struct ReadResult {
  enum struct Code : u32 {
    Ok,

    EOF,

    // Generic error, no specifics known
    Error,
  };

  // Number of bytes read. May be not 0 even if code != Ok
  usz n;

  Code code;

  let ReadResult() noexcept : n(0), code(Code::Ok) {}
  let ReadResult(usz k) noexcept : n(k), code(Code::Ok) {}
  let ReadResult(Code c) noexcept : n(0), code(c) {}
  let ReadResult(Code c, usz k) noexcept : n(k), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_eof() const noexcept { return code == Code::EOF; }
  method bool is_err() const noexcept {
    return code != Code::Ok && code != Code::EOF;
  }
};

struct FileReadResult {
  enum struct Code : u8 {
    Ok,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  mc data;

  Code code;

  let FileReadResult(mc d) noexcept : data(d), code(Code::Ok) {}
  let FileReadResult(Code c) noexcept : data(mc()), code(c) {}
  let FileReadResult(mc d, Code c) noexcept : data(d), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

struct WriteResult {
  enum struct Code : u32 {
    Ok,

    // Generic error, no specifics known
    Error,

    Flush,
  };

  // Number of bytes written
  usz n;

  Code code;

  let WriteResult() noexcept : n(0), code(Code::Ok) {}
  let WriteResult(usz k) noexcept : n(k), code(Code::Ok) {}
  let WriteResult(Code c) noexcept : n(0), code(c) {}
  let WriteResult(Code c, usz k) noexcept : n(k), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn ReadResult read(FileDescriptor fd, mc c) noexcept;

fn ReadResult read_all(FileDescriptor fd, mc c) noexcept {
  var usz i = 0;
  while (i < c.len) {
    const ReadResult r = read(fd, c.slice_from(i));
    i += r.n;

    if (!r.is_ok()) {
      return ReadResult(r.code, i);
    }
  }

  return ReadResult(c.len);
}

fn WriteResult write(FileDescriptor fd, mc c) noexcept;

fn WriteResult write_all(FileDescriptor fd, mc c) noexcept {
  var usz i = 0;
  while (i < c.len) {
    const WriteResult r = write(fd, c.slice_from(i));
    i += r.n;

    if (r.is_err()) {
      return WriteResult(r.code, i);
    }
  }

  return WriteResult(c.len);
}

struct BufFileWriter {
  // Internal buffer for storing raw bytes before
  // commiting accumulated writes to file
  bb buf;

  // File descriptor
  //
  // Actual meaning of this number is platform-specific
  FileDescriptor fd;

  let BufFileWriter() noexcept {}

  let BufFileWriter(FileDescriptor f, mc c) noexcept : buf(bb(c)), fd(f) {
    must(c.len != 0);
  }

  method WriteResult write(mc c) noexcept {
    var usz i = 0;
    while (i < c.len) {
      var usz n = buf.write(c.slice_from(i));
      i += n;
      if (n == 0) {
        var WriteResult r = flush();
        if (r.is_err()) {
          return WriteResult(r.code, i);
        }
      }
    }

    return WriteResult(c.len);
  }

  // write line feed (aka "newline") character
  method WriteResult lf() noexcept {
    if (buf.rem() == 0) {
      const WriteResult wr = flush();
      if (wr.is_err()) {
        return WriteResult(WriteResult::Code::Flush);
      }
    }

    buf.lf();
    return WriteResult(1);
  }

  // Commits stored writes to underlying file (designated by file descriptor)
  method WriteResult flush() noexcept {
    var WriteResult r = write_all(fd, buf.head());
    if (r.is_err()) {
      return r;
    }

    buf.reset();
    return r;
  }

  method CloseResult close() noexcept {
    flush();
    return fs::close(fd);
  }
};

}  // namespace fs



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

namespace container {

// Hash table of static size with no collisions. This is achived
// by handpicking starting seed for hash function
template <typename T>
struct FlatMap {
  // Key-value pair that can be added to map. Mainly used
  // to populate a map with many values at once
  struct Pair {
    str key;
    T value;

    let Pair(str k, T v) noexcept : key(k), value(v) {}
  };

  // Item stored in map. Returned when get method is used
  struct Item {
    // value that was placed in map by add method
    T value;

    // true if item is actually stored in map, i.e.
    // it is map item occupation flag
    bool ok;

    // constructs empty item
    let Item() noexcept : value(T()), ok(false) {}

    let Item(T v) noexcept : value(v), ok(true) {}
  };

  // Internal structure used for storing items in map
  struct Entry {
    // key hash of stored item
    u64 hash;

    // key length of stored item
    usz len;

    Item item;
  };

  // continuous chunk of stored entries
  chunk<Entry> entries;

  u64 mask;

  u64 seed;

  // number of elements stored in map
  usz len;

  // minimal length of key stored in map
  usz min_key_len;

  // maximum length of key stored in map
  usz max_key_len;

  let FlatMap() noexcept
      : entries(chunk<Entry>()),
        mask(0),
        seed(0),
        len(0),
        min_key_len(0),
        max_key_len(0) {}

  let FlatMap(usz cap, u64 m, u64 s) noexcept
      : entries(chunk<Entry>(nil, cap)),
        mask(m),
        seed(s),
        len(0),
        min_key_len(0),
        max_key_len(0) {
    //
    init();
  }

  method void init() noexcept {
    const usz cap = entries.len;
    entries = mem::calloc<Entry>(cap);
  }

  method bool is_nil() noexcept { return entries.is_nil(); }

  method bool is_empty() noexcept { return len == 0; }

  method void free() noexcept { mem::free(entries); }

  method u64 hash(str key) noexcept { return hash::map::compute(seed, key); }

  method usz determine_pos(u64 h) noexcept { return h & mask; }

  // Returns true if item was successfully added to map.
  // Returns false if corresponding cell was already
  // occupied in map
  method bool add(str key, T value) noexcept {
    const usz h = hash(key);
    const usz pos = determine_pos(h);

    const Entry entry = entries.ptr[pos];

    if (entry.item.ok) {
      return false;
    }

    entries.ptr[pos] = {
        .hash = h,
        .len = key.len,
        .item = Item(value),
    };

    if (key.len > max_key_len) {
      max_key_len = key.len;
    }

    if (is_empty()) {
      min_key_len = key.len;
    } else if (key.len < min_key_len) {
      min_key_len = key.len;
    }

    len += 1;

    return true;
  }

  method bool add(Pair pair) noexcept { return add(pair.key, pair.value); }

  method Item get(str key) noexcept {
    if ((key.len < min_key_len) || (key.len > max_key_len)) {
      return Item();
    }

    const usz h = hash(key);
    const usz pos = determine_pos(h);
    const Entry entry = entries.ptr[pos];

    if (!entry.item.ok || entry.len != key.len || entry.hash != h) {
      return Item();
    }

    return entry.item;
  }

  method void clear() noexcept {
    entries.clear();

    len = 0;
    min_key_len = 0;
    max_key_len = 0;
  }

  // Adds elements to map one by one until all elements are
  // added or map is unable to hold next element
  //
  // Returns true if all elements were successfully added
  // to map. When true is returned function guarantees that
  // number of supplied literals equals resulting map.len
  //
  // Returns false as soon as at least one element was not
  // successfully added. In this case number of elements
  // actually stored inside a map is unpredictable
  fn bool populate(chunk<Pair> pairs) noexcept {
    for (usz i = 0; i < pairs.len; i += 1) {
      const Pair pair = pairs.ptr[i];
      const bool ok = add(pair);

      if (!ok) {
        return false;
      }
    }

    return true;
  }

  // print empty and occupied elements to buffer
  // in illustrative way
  method mc visualize(mc c) noexcept {
    var bb buf = bb(c);

    const usz row_len = 64;

    var Entry* p = entries.ptr;
    for (usz i = 0; i < entries.len / row_len; i += 1) {
      for (usz j = 0; j < row_len; j += 1) {
        const Entry entry = p[j];

        if (entry.item.ok) {
          buf.write('X');
        } else {
          buf.write('_');
        }
      }

      p += row_len;
      buf.lf();
      buf.lf();
    }

    return buf.head();
  }
};

// Allocates a map of specified (cap) size on the heap and
// picks a special seed for map hashing function which allows
// to store all strings from literals chunk in map without
// collisions
//
// Argument cap must be a power of 2. Argument mask must correspond
// to that power with respective number of 1's in lower bits.
// Number of supplied literals must be less than specified capacity.
// It is recommended to pick capacity approximately five times
// greater than number of literals
template <typename T>
fn FlatMap<T> fit_into_flat_map(
    usz cap,
    u64 mask,
    chunk<typename FlatMap<T>::Pair> pairs) noexcept {
  //
  var FlatMap<T> m = FlatMap<T>(cap, mask, 0);

  for (u64 j = 0; j < 100000; j += 1) {
    const bool ok = m.populate(pairs);

    if (ok) {
      return m;
    }

    m.clear();
    m.seed += 1;
  }

  m.free();
  return FlatMap<T>();
}

template <typename T>
struct CircularBuffer {
  // Pointer to buffer starting position
  T* ptr;

  // Buffer capacity i.e. maximum number of elements
  // it can hold
  usz cap;

  // Number of elements currently stored inside buffer
  usz len;

  // Tip position. Next added element will be placed
  // inside with this offset
  usz pos;

  method void append(T elem) noexcept {
    ptr[pos] = elem;

    if (pos == cap) {
      pos = 0;
    } else {
      pos += 1;
    }

    if (len == cap) {
      return;
    }

    len += 1;
  }

  method T pop() noexcept {
    must(len != 0);

    if (len <= pos) {
      const usz i = pos - len;
      len -= 1;
      return ptr[i];
    }

    const usz i = cap - len + pos;
    len -= 1;
    return ptr[i];
  }
};

}  // namespace container

namespace bufio {

// Wraps a given writer by adding a buffer to it
//
// Clients must supply a writer that implements the following list
// of methods:
//
//  - write_all(mc c)
//  - close()
template <typename T>
struct Writer {
  // Internal buffer for storing raw bytes before
  // commiting accumulated writes to file
  bb buf;

  // Underlying writer that is being wrapped
  T w;

  // Create a buffered writer from a given writer and a supplied
  // buffer. Buffer is used as long as writer lives
  let Writer(T w, mc c) noexcept : buf(bb(c)), w(w) { must(c.len != 0); }

  method fs::WriteResult write(mc c) noexcept {
    if (c.len == 0) {
      return fs::WriteResult();
    }
    const usz n = buf.write(c);
    if (n != 0) {
      return fs::WriteResult(n);
    }

    const fs::WriteResult r = flush();
    if (r.is_err()) {
      return fs::WriteResult(fs::WriteResult::Code::Flush, n);
    }
    const usz n2 = buf.write(c);
    return fs::WriteResult(n2);
  }

  method fs::WriteResult write_all(mc c) noexcept {
    var usz i = 0;
    while (i < c.len) {
      const usz n = buf.write(c.slice_from(i));
      i += n;
      if (n == 0) {
        const fs::WriteResult r = flush();
        if (r.is_err()) {
          return fs::WriteResult(fs::WriteResult::Code::Flush, i);
        }
      }
    }

    return fs::WriteResult(c.len);
  }

  method void print(str s) noexcept { write_all(s); }

  method void println(str s) noexcept {
    print(s);
    lf();
  }

  // Write line feed (aka "newline") character
  method fs::WriteResult lf() noexcept {
    if (buf.rem() == 0) {
      const fs::WriteResult wr = flush();
      if (wr.is_err()) {
        return fs::WriteResult(fs::WriteResult::Code::Flush);
      }
    }

    buf.lf();
    return fs::WriteResult(1);
  }

  // Commits stored writes to underlying writer
  method fs::WriteResult flush() noexcept {
    const fs::WriteResult r = w.write_all(buf.head());
    if (r.is_err()) {
      return r;
    }

    buf.reset();
    return r;
  }

  // Flush the buffer and close underlying writer
  method fs::CloseResult close() noexcept {
    flush();
    return w.close();
  }
};

}  // namespace bufio
