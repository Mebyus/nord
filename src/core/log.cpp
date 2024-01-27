namespace coven::log {

struct Logger {
  static const usz buf_size = 1 << 13;
  // static const usz ts_prefix_size = 12;

  enum struct Level : u8 {
    All = 0,

    // Only levels from this block can represent
    // levels of logged messages
    Debug,
    Info,
    Warn,
    Error,
    Assert,

    Nothing,
  };

  u8 buf[buf_size];

  // Buffer for writing timestamp
  // u8 ts_buf[ts_prefix_size];

  // Output buffer
  fs::BufFileWriter writer;

  // Starting Tick when Logger was created
  // time::Tick start;

  Level level;

  // Use only for global variables. Produced Logger is invalid and
  // must be overwritten later using any other constructor
  let Logger() noexcept : level(Level::All), in_progress(false) {}

  let Logger(Level level) noexcept : level(level), in_progress(false) {}

  let Logger(str filename) noexcept : level(Level::All), in_progress(false) {
    init(filename);
  }

  let Logger(str filename, Level level) noexcept : level(level), in_progress(false) {
    init(filename);
  }

  method void init(str filename) noexcept {
    const io::OpenResult r = io::create(filename);
    if (r.is_err()) {
      return;
    }

    writer = fs::BufFileWriter(r.fd, mc(buf, buf_size));
  }

  method void flush() noexcept {
    end();
    writer.flush();
  }

  // Continue current message. Has no effect if level of current message is
  // less than level of the logger
  method void append(str s) noexcept {
    if (is_visible()) {
      write(s);
    }
  }

  method void debug(str s) noexcept {
    if (!start(Level::Debug)) {
      return;
    }

    const str prefix = static_string("  [debug] ");
    // write_timestamp();
    write(prefix);
    write(s);
  }

  method void info(str s) noexcept {
    if (!start(Level::Info)) {
      return;
    }

    const str prefix = static_string("   [info] ");
    // write_timestamp();
    write(prefix);
    write(s);
  }

  method void warn(str s) noexcept {
    if (!start(Level::Warn)) {
      return;
    }

    const str prefix = static_string("   [warn] ");
    // write_timestamp();
    write(prefix);
    write(s);
  }

  method void error(str s) noexcept {
    if (!start(Level::Error)) {
      return;
    }

    const str prefix = static_string("  [error] ");
    // write_timestamp();
    write(prefix);
    write(s);
  }

 private:
  // Level of current message being written
  Level msg_level;

  // Flag to manage newline placement when new message starts
  bool in_progress;

  method void write(str s) noexcept { writer.write(s); }

  method void lf() noexcept { writer.lf(); }

  method bool is_visible() noexcept { return msg_level > level; }

  // Returns true if provided level of new message is visible
  // with the current logging level
  method bool start(Level l) noexcept {
    end();

    msg_level = l;
    if (is_visible()) {
      in_progress = true;
      return true;
    }

    return false;
  }

  method void end() noexcept {
    if (in_progress) {
      lf();
    }
    in_progress = false;
  }

  // method void write_timestamp() noexcept {
  //     var time::Interval i = time::since(start);

  //     var mc c = mc(ts_buf, ts_prefix_size);
  //     c.pad_format_dec(i.as_milli());
  //     write(c);
  // }
};

};  // namespace coven::log
