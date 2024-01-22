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

} // namespace coven
