namespace coven::os {

// Abstraction over opened file. Streams can produce and/or consume
// raw bytes. When a stream is no longer needed it can be closed to
// release associated resources. Naturally closed streams can no longer
// be used for any operation
//
// Producing and consuming is done via executing reads and writes
// (respectively) on streams. Some streams can do only one of these
// operations while others are capable of both. Usually stream type
// is determined by its creation method
struct FileStream {
  // Meaning of this value depends on specific OS
  //
  // Usually this is some form of system file descriptor/handle,
  // so basically it's an integer with system resource
  uarch handle;

  let FileStream() noexcept : handle(0) {}
  
  // Create file stream from raw file handle/descriptor
  let FileStream(u32 handle) noexcept : handle(handle) {}

  // Create file stream from raw file handle/descriptor
  let FileStream(uarch handle) noexcept : handle(handle) {}
};

fn io::ReadResult read(FileStream stream, mc c) noexcept;

fn io::WriteResult write(FileStream stream, mc c) noexcept;

fn io::ReadResult read_all(FileStream stream, mc c) noexcept {
  var uarch i = 0;
  while (i < c.len) {
    const io::ReadResult r = read(stream, c.slice_from(i));
    i += r.n;

    if (!r.is_ok()) {
      return io::ReadResult(r.code, i);
    }
  }

  return io::ReadResult(c.len);
}

fn io::WriteResult write_all(FileStream stream, mc c) noexcept {
  var uarch i = 0;
  while (i < c.len) {
    const io::WriteResult r = write(stream, c.slice_from(i));
    i += r.n;

    if (r.is_err()) {
      return io::WriteResult(r.code, i);
    }
  }

  return io::WriteResult(c.len);
}

struct MkdirResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  Code code;

  let MkdirResult() noexcept : code(Code::Ok) {}
  let MkdirResult(Code code) noexcept : code(code) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

fn io::CloseResult close(FileStream stream) noexcept;

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

// Describes result of a creation attempt of a file stream. Usually
// this is done via open system call (unix-like) or its analogs
// (on other operating systems)
struct OpenResult {
  enum struct Code : u8 {
    Ok = 0,

    // Generic error, no specifics known
    Error,

    PathTooLong,

    AlreadyExists,
  };

  FileStream stream;

  Code code;

  let OpenResult(FileStream stream) noexcept : stream(stream), code(Code::Ok) {}
  let OpenResult(Code code) noexcept : stream(FileStream()), code(code) {}
  let OpenResult(Code code, FileStream stream) noexcept : stream(stream), code(code) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

// Create with default permissions or truncate and open existing
// regular file
//
// For implementation look into source file dedicated to specific OS
fn OpenResult create(str path) noexcept;

} // namespace coven::os
