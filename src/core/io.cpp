namespace coven::io {

typedef uarch FileHandle;

struct OpenResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  FileHandle fd;

  Code code;

  let OpenResult(FileHandle f) noexcept : fd(f), code(Code::Ok) {}
  let OpenResult(Code c) noexcept : fd(0), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

struct MkdirResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  Code code;

  let MkdirResult(FileHandle f) noexcept : fd(f), code(Code::Ok) {}
  let MkdirResult(Code c) noexcept : fd(0), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn OpenResult create(str filename) noexcept;

struct CloseResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    InvalidHandle,
    InputOutputError,
    NoSpaceLeftOnDevice,
    DiskQuotaExceeded,
  };

  Code code;

  let CloseResult() noexcept : code(Code::Ok) {}
  let CloseResult(Code c) noexcept : code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn CloseResult close(FileHandle fd) noexcept;

struct ReadResult {
  enum struct Code : u8 {
    Ok = 0,

    EOF,

    // Generic error, no specifics known
    Error,
  };

  // Number of bytes read. May be not 0 even if code != Ok
  uarch n;

  Code code;

  let ReadResult() noexcept : n(0), code(Code::Ok) {}
  let ReadResult(uarch k) noexcept : n(k), code(Code::Ok) {}
  let ReadResult(Code c) noexcept : n(0), code(c) {}
  let ReadResult(Code c, uarch k) noexcept : n(k), code(c) {}

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
  uarch n;

  Code code;

  let WriteResult() noexcept : n(0), code(Code::Ok) {}
  let WriteResult(uarch k) noexcept : n(k), code(Code::Ok) {}
  let WriteResult(Code c) noexcept : n(0), code(c) {}
  let WriteResult(Code c, uarch k) noexcept : n(k), code(c) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn ReadResult read(FileHandle fd, mc c) noexcept;

fn ReadResult read_all(FileHandle fd, mc c) noexcept {
  var uarch i = 0;
  while (i < c.len) {
    const ReadResult r = read(fd, c.slice_from(i));
    i += r.n;

    if (!r.is_ok()) {
      return ReadResult(r.code, i);
    }
  }

  return ReadResult(c.len);
}

fn WriteResult write(FileHandle fd, mc c) noexcept;

fn WriteResult write_all(FileHandle fd, mc c) noexcept {
  var uarch i = 0;
  while (i < c.len) {
    const WriteResult r = write(fd, c.slice_from(i));
    i += r.n;

    if (r.is_err()) {
      return WriteResult(r.code, i);
    }
  }

  return WriteResult(c.len);
}

}  // namespace coven::io
