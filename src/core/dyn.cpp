namespace coven {

fn internal inline uarch determine_grow_amount(uarch cap, uarch len) noexcept {
  if (len <= cap) {
    const uarch size_threshold = 1 << 16;
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

  let DynBuffer(uarch n) noexcept : buf(buffer<T>()) { init(n); }

  method void init(uarch n) noexcept {
    if (n == 0) {
      return;
    }
    buf = buffer<T>(mem::alloc<T>(n));
  }

  // Increase buffer capacity by at least n elements
  method void grow(uarch n) noexcept {
    if (n == 0) {
      return;
    }

    if (buf.is_nil()) {
      buf = buffer<T>(mem::alloc<T>(n));
      return;
    }

    const uarch len = buf.len;

    buf = buffer<T>(mem::realloc<T>(buf.body(), buf.cap + n));
    buf.len = len;
  }

  method void append(T elem) noexcept {
    if (buf.rem() >= 1) {
      buf.unsafe_append(elem);
      return;
    }

    const uarch n = determine_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_append(elem);
  }

  // Insert single element at specified index. All elements with
  // greater or equal indices will be shifted by one position
  // to the right. This operation increases buffer length by 1
  method void insert(uarch i, T x) noexcept {
    must(i <= buf.len);

    if (buf.rem() >= 1) {
      buf.unsafe_insert(i, x);
      return;
    }

    const uarch n = determine_bytes_grow_amount(buf.cap, 1);
    grow(n);

    buf.unsafe_insert(i, x);
  }

  // Remove single element at specified index. All active elements with
  // greater indices will be shifted by one position to the left.
  // This operation decreases buffer length by 1
  method void remove(uarch i) noexcept {
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

  method uarch len() const noexcept { return buf.len; }

  method chunk<T> head() const noexcept { return buf.head(); }
};

} // namespace coven
