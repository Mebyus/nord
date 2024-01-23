namespace coven {

// Memory Chunk
//
// Represents continuous region in memory in form of raw bytes.
// Chunk by definition is nil when number of bytes in it equals zero
//
// Meaning of chunk contents are dependant on its specific usage. For example
// bytes can represent a utf-8 string, bitmap of an image or a network packet.
// Users should specify meaning of chunk in place where they use it
//
// Chunk can point to static, stack or heap allocated memory. It is
// responsibility of the user to keep track of different usages and deallocate
// backing memory
//
// Depending on particular usage chunk may either be:
//  1) a view into memory owned by someone else
//  2) owned memory region
struct mc {
  // Pointer to first byte in chunk
  u8* ptr;

  // Number of bytes in chunk
  uarch len;

  // Create nil memory chunk
  let constexpr mc() noexcept : ptr(nil), len(0) {}

  let mc(u8* bytes, uarch n) noexcept : ptr(bytes), len(n) {}

  u8& operator[](uarch i) {
    must(i < len);
    must(ptr != nil);

    return ptr[i];
  }

  // Fill chunk with zero bytes
  method void clear() noexcept;

  // Fill chunk with specified byte
  method void fill(u8 x) noexcept;

  // Parameters must satisfy 0 <= start <= end <= len
  //
  // Method does not perform any safety checks
  //
  // If start = end then resulting slice will be nil
  method mc slice(uarch start, uarch end) const noexcept {
    return mc(ptr + start, end - start);
  }

  method mc slice_from(uarch start) const noexcept { return slice(start, len); }

  method mc slice_to(uarch end) const noexcept { return slice(0, end); }

  // Slice a portion of memory chunk starting from the start of
  // original slice
  //
  // In contrast with slice_to method argument specifies how many
  // bytes should be cut from the end of original slice
  method mc slice_down(uarch size) const noexcept { return slice_to(len - size); }

  // Return a slice from original chunk that contains at most
  // specified number of bytes
  method mc crop(uarch size) const noexcept { return slice_to(min(len, size)); }

  method bool is_nil() const noexcept { return len == 0; }

  method uarch write(u8* bytes, uarch n) noexcept {
    return write(mc(bytes, n));
  }

  // Write (copy) given chunk to this one. Writing starts from
  // first byte of both chunks
  //
  // Returns number of bytes written. Return value may be smaller than
  // expected or even 0 if chunk does not have enough space to
  // hold given number of bytes
  method uarch write(mc c) noexcept { 
    const uarch n = min(c.len, len);
    if (n == 0) {
      return 0;
    }

    unsafe_write(c.slice_to(n));
    return w;
  }

  // Write single byte to chunk
  method uarch write(u8 x) noexcept {
    if (is_nil()) {
      return 0;
    }
    unsafe_write(x);
    return 1;
  }

  method void unsafe_write(u8* bytes, uarch n) noexcept { unsafe_write(mc(bytes, n)); }

  method void unsafe_write(mc c) noexcept { copy(c.ptr, ptr, c.len); }

  // Write single byte to chunk
  method void unsafe_write(u8 x) noexcept { ptr[0] = x; }

  method void unsafe_write(char x) noexcept { unsafe_write(cast(u8, x)); }

  // Reverse bytes in chunk
  method void reverse() noexcept {
    if (is_nil()) {
      return;
    }

    var uarch i = 0;
    var uarch j = len - 1;
    for (; i < j; i += 1, j -= 1) {
      swap(ptr[i], ptr[j]);
    }
  }

  method uarch fmt_dec(u8 x) noexcept;

  // Same as format_dec, but unused bytes will be filled with padding
  method uarch pad_fmt_dec(u64 x) noexcept {
    var uarch w = fmt_dec(x);
    if (w == 0) {
      return 0;
    }
    if (w == len) {
      return w;
    }
    slice_from(w).fill(' ');
    return len;
  }

  // Write specified byte repeated n times to memory chunk
  method uarch write_repeat(uarch n, u8 x) noexcept {
    n = min(n, len);
    unsafe_write_repeat(n, x);
    return n;
  }

  method void unsafe_write_repeat(uarch n, u8 x) noexcept {
    for (uarch i = 0; i < n; i += 1) {
      ptr[i] = x;
    }
  }
};

method void mc::clear() noexcept {
  must(len != 0);
  must(ptr != nil);

  for (uarch i = 0; i < len; i += 1) {
    ptr[i] = 0;
  }
}

method void mc::fill(u8 x) noexcept {
  must(len != 0);
  must(ptr != nil);

  for (uarch i = 0; i < len; i += 1) {
    ptr[i] = x;
  }
}

// Alias for code documentation of intented memory chunk meaning
//
// String type indicates that bytes stored in chunk should (but may not)
// represent utf-8 encoded string
//
// Note that there is no guarantee that bytes represent correct
// sequence of unicode code points. This type only serves as a hint
// to reader of source code
typedef mc str;

// Shorthand for string literal to memory chunk conversion
#define static_string(s) str((u8*)u8##s, sizeof(u8##s) - 1)

// Stores elements (of the same type) in continuous memory region
// and their number together as a single data structure
template <typename T>
struct chunk {
  // Pointer to first element in chunk
  T* ptr;

  // Number of elements in chunk
  usz len;

  let chunk() noexcept : ptr(nil), len(0) {}

  let chunk(T* elems, usz n) noexcept : ptr(elems), len(n) {}

  method bool is_nil() const noexcept { return len == 0; }

  method mc as_mc() const noexcept { return mc(cast(u8*, ptr), size()); }

  method usz size() const noexcept { return chunk_size(T, len); }

  method void clear() noexcept { as_mc().clear(); }
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

  // Returns memory chunk with stored bytes. Zero terminator
  // is not included
  method str as_str() const noexcept { return str(ptr, len); }

  // Returns memory chunk with stored bytes. Includes zero terminator
  // in its length
  method mc with_null() const noexcept { return mc(ptr, len + 1); }
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

} // namespace coven
