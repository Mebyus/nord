namespace coven::io {

struct ReadResult {
  enum struct Code : u8 {
    Ok = 0,

    // End-Of-File indicator. If read returns result with this code
    // than it means the associated resource (from which read was
    // performed) is exhausted. All subsequent reads from exhaused
    // resource will return EOF result with zero bytes read
    //
    // Note that first EOF result can contain non-zero number of bytes.
    // Also it should be clarified that this result code is not considered
    // as error, however it differs from regular read with Ok code
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
  let CloseResult(Code code) noexcept : code(code) {}

  method bool is_ok() const noexcept { return code == Code::Ok; }
  method bool is_err() const noexcept { return code != Code::Ok; }
};

}  // namespace coven::io
