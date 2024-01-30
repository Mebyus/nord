namespace coven {

typedef cont::FlatMap<uarch>::Pair IndexedWord;

fn DynBuffer<IndexedWord> split_and_index_words(str text) noexcept {
  var DynBuffer<IndexedWord> buf = DynBuffer<IndexedWord>();

  var uarch n = 0;  // word number
  var uarch start = 0;
  var uarch i = 0;
  for (; i < text.len; i += 1) {
    const u8 c = text.ptr[i];

    if (!fmt::is_simple_whitespace(c)) {
      continue;
    }

    if (start != i) {
      // reached end of current word
      var str word = text.slice(start, i);
      buf.append(IndexedWord(word, n));
      n += 1;
    }
    start = i + 1;
  }

  if (start != i) {
    // save possible last word at the end of text
    // that has no whitespace after it
    var str word = text.slice(start, i);
    buf.append(IndexedWord(word, n));
  }

  return buf;
}

struct CapSeedPair {
  u64 seed;
  uarch cap;

  bool ok;
};

fn CapSeedPair find_best_cap_and_seed(chunk<IndexedWord> words) noexcept {
  var uarch cap = words.len << 1;
  cap = bits::upper_power_of_two(cast(u32, cap));

  var cont::FlatMap<uarch> m =
      cont::fit_into_flat_map<uarch>(cap, cap - 1, words);

  if (m.len == 0) {
    return CapSeedPair{.seed = 0, .cap = 0, .ok = false};
  }

  return CapSeedPair{.seed = m.seed, .cap = cap, .ok = true};
}

} // namespace coven

using namespace coven;

fn i32 main(i32 argc, u8** argv) noexcept {
  if (argc < 2) {
    return 1;
  }
  const cstr filename = cstr(argv[1]);
  var os::FileReadResult rr = os::read_file(filename.as_str());
  if (rr.is_err()) {
    return 1;
  }

  var DynBuffer<IndexedWord> words = split_and_index_words(rr.data);
  const CapSeedPair pair = find_best_cap_and_seed(words.head());

  if (!pair.ok) {
    stdout_write_all(
        macro_static_str("failed to pick cap and seed for given input\n"));
    return 1;
  }

  var u8 scratch[256] dirty;
  var bb buf = bb(scratch, sizeof(scratch));
  buf.write(macro_static_str("len  = "));
  buf.fmt_dec(words.len());
  buf.lf();
  buf.write(macro_static_str("cap  = "));
  buf.fmt_dec(pair.cap);
  buf.lf();
  buf.write(macro_static_str("seed = "));
  buf.fmt_dec(pair.seed);
  buf.lf();
  stdout_write_all(buf.head());
  return 0;
}
