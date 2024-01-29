namespace coven::cont {

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
    uarch len;

    Item item;
  };

  // continuous chunk of stored entries
  chunk<Entry> entries;

  u64 mask;

  u64 seed;

  // number of elements stored in map
  uarch len;

  // minimal length of key stored in map
  uarch min_key_len;

  // maximum length of key stored in map
  uarch max_key_len;

  let FlatMap() noexcept
      : entries(chunk<Entry>()),
        mask(0),
        seed(0),
        len(0),
        min_key_len(0),
        max_key_len(0) {}

  let FlatMap(uarch cap, u64 m, u64 s) noexcept
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
    const uarch cap = entries.len;
    entries = mem::calloc<Entry>(cap);
  }

  method bool is_nil() noexcept { return entries.is_nil(); }

  method bool is_empty() noexcept { return len == 0; }

  method void free() noexcept { mem::free(entries); }

  method u64 hash(str key) noexcept { return hash::map::compute(seed, key); }

  method uarch determine_pos(u64 h) noexcept { return h & mask; }

  // Returns true if item was successfully added to map.
  // Returns false if corresponding cell was already
  // occupied in map
  method bool add(str key, T value) noexcept {
    const uarch h = hash(key);
    const uarch pos = determine_pos(h);

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

    const uarch h = hash(key);
    const uarch pos = determine_pos(h);
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
    for (uarch i = 0; i < pairs.len; i += 1) {
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

    const uarch row_len = 64;

    var Entry* p = entries.ptr;
    for (uarch i = 0; i < entries.len / row_len; i += 1) {
      for (uarch j = 0; j < row_len; j += 1) {
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
    uarch cap,
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
  uarch cap;

  // Number of elements currently stored inside buffer
  uarch len;

  // Tip position. Next added element will be placed
  // inside with this offset
  uarch pos;

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
      const uarch i = pos - len;
      len -= 1;
      return ptr[i];
    }

    const uarch i = cap - len + pos;
    len -= 1;
    return ptr[i];
  }
};

}  // namespace container