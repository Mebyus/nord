namespace coven::bufio {

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
  let Writer(T w, mc c) noexcept : buf(bb(c)), w(w) { 
    must(c.len != 0);
    must(c.ptr != nil);
  }

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

}  // namespace coven::bufio
