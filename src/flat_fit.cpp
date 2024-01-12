#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "core.hpp"

#include "core_linux.cpp"

struct IndexedWord {
  str word;
  usz index;
};

fn DynBuffer<IndexedWord> split_and_index_words(str text) noexcept {
  var DynBuffer<IndexedWord> buf = DynBuffer<IndexedWord>();

  var usz n = 0; // word number
  var usz start = 0;
  var usz i = 0;
  for (; i < text.len; i += 1) {
    const u8 c = text.ptr[i];

    if (!text::is_simple_whitespace(c)) {
      continue;
    }

    if (start != i) {
      // reached end of current word
      var str word = text.slice(start, i);
      buf.append(IndexedWord{.word = word, .index = n});
      n += 1;
    }
    start = i + 1;
  }

  if (start != i) {
    // save possible last word at the end of text
    // that has no whitespace after it
    var str word = text.slice(start, i);
    buf.append(IndexedWord{.word = word, .index = n});
  }

  return buf;
}

fn u64 find_best_cap_and_seed(chunk<IndexedWord> words) noexcept {
  var usz cap = words.len << 1;
  cap = bits::upper_power_of_two(cast(u32, cap));
  return cap;
}

fn i32 main(i32 argc, u8** argv) noexcept {
  if (argc < 2) {
    return 1;
  }
  var cstr filename = cstr(argv[1]);
  var fs::FileReadResult rr = fs::read_file(filename.as_str());
  if (rr.is_err()) {
    return 1;
  }

  var DynBuffer<IndexedWord> words = split_and_index_words(rr.data);
  const u64 cap = find_best_cap_and_seed(words.head());

  var u8 scratch[32] dirty;
  var bb buf = bb(scratch, sizeof(scratch));
  buf.fmt_dec(cap);
  buf.lf();
  stdout_write_all(buf.head());
  return 0;
}
