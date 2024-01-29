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
  fmt::Buffer buf;

  // Underlying writer that is being wrapped
  T w;

  let Writer() noexcept {}

  // Create a buffered writer from a given writer and a supplied
  // buffer. Buffer is used as long as writer lives
  let Writer(T writer, mc c) noexcept : buf(fmt::Buffer(c)), w(writer) { 
    must(c.len != 0);
    must(c.ptr != nil);
  }

  method io::WriteResult write(mc c) noexcept {
    if (c.len == 0) {
      return io::WriteResult();
    }
    const uarch n = buf.write(c);
    if (n != 0) {
      return io::WriteResult(n);
    }

    const io::WriteResult r = flush();
    if (r.is_err()) {
      return io::WriteResult(io::WriteResult::Code::Flush, n);
    }
    const uarch n2 = buf.write(c);
    return io::WriteResult(n2);
  }

  method io::WriteResult write_all(mc c) noexcept {
    var uarch i = 0;
    while (i < c.len) {
      const uarch n = buf.write(c.slice_from(i));
      i += n;
      if (n == 0) {
        const io::WriteResult r = flush();
        if (r.is_err()) {
          return io::WriteResult(io::WriteResult::Code::Flush, i);
        }
      }
    }

    return io::WriteResult(c.len);
  }

  method void print(str s) noexcept { write_all(s); }

  method void println(str s) noexcept {
    print(s);
    lf();
  }

  // Write line feed (aka "newline") character
  method io::WriteResult lf() noexcept {
    if (buf.rem() == 0) {
      const io::WriteResult wr = flush();
      if (wr.is_err()) {
        return io::WriteResult(io::WriteResult::Code::Flush);
      }
    }

    buf.lf();
    return io::WriteResult(1);
  }

  // Commits stored writes to underlying writer
  method io::WriteResult flush() noexcept {
    const io::WriteResult r = w.write_all(buf.head());
    if (r.is_err()) {
      return r;
    }

    buf.reset();
    return r;
  }

  // Flush the buffer and close underlying writer
  method io::CloseResult close() noexcept {
    flush();
    return w.close();
  }
};

}  // namespace coven::bufio
