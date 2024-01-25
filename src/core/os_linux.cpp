namespace coven::os::linux {

internal const u32 stdin_fd = 0;
internal const u32 stdout_fd = 1;
internal const u32 stderr_fd = 2;

fn inline io::OpenResult open(cstr path, u32 flags, u32 mode) noexcept {
  var syscall::Result r dirty;
  do {
    r = syscall::open(path.ptr, flags, mode);
  } while (r.err == syscall.EINTR);

  if (r.is_ok()) {
    return io::OpenResult(r.val);
  }

  // dispatch error
}

fn inline io::CloseResult::Code dispatch_close_error(syscall::Error err) noexcept {
  switch (err) {
  case syscall::Error::EBADF: 
    // Not a valid open file descriptor.
    return io::CloseResult::Code::InvalidHandle;

  case syscall::Error::EIO:
    // An I/O error occurred.
    return io::CloseResult::Code::InputOutputError;

  // ENOSPC, EDQUOT
  // On NFS, these errors are not normally reported against the first write
  // which exceeds the available storage space, but instead against
  // a subsequent write, fsync, or close
  case syscall::Error::ENOSPC:
    return io::CloseResult::Code::NoSpaceLeftOnDevice;
  case syscall::Error::EDQUOT:
    return io::CloseResult::Code::DiskQuotaExceeded;

  default:
    // Unknown error code
    return io::CloseResult::Code::Error;
  }
}

fn io::CloseResult close(io::FileHandle handle) noexcept {
  var syscall::Result r dirty;
  do {
    r = syscall::close(cast(u32, handle));
  } while (r.err == syscall.EINTR);

  if (r.is_ok()) {
    return io::CloseResult();
  }

  return io::CloseResult(dispatch_close_error(r.err));
}

fn io::ReadResult read(io::FileHandle handle, mc buf) noexcept {
  // TODO: check too large buffer size (at most 0x7ffff000)
  var syscall::Result r dirty;
  do {
    r = syscall::read(handle, buf.ptr, buf.len);
  } while (r.err == syscall.EINTR);

  if (r.is_ok()) {
    return io::ReadResult(r.val);
  }

  // dispatch error
}

}  // namespace coven::os::linux

namespace coven::os {

// Represents a one-way consumer of byte stream. Bytes can be written
// to it, but cannot be read from
//
// This consumer is usually tied to some external resource (open file,
// socket, pipe, network connection, etc.) and thus write operation can
// result in error which is outside of current program control. From this
// perspective encountering errors when interacting with this object is
// considered normal behaviour and should not be treated by clients as
// something extraordinary and unexpected. Instead errors produced on
// writes should be checked and handled accordingly to their nature
//
// Note that this implementation does unbuffered writes. To make buffered
// version use bufio::Writer
struct Sink {
  fs::FileHandle fd;

  let Sink(fs::FileHandle fd) noexcept : fd(fd) {}

  method fs::WriteResult write(mc c) noexcept { return fs::write(fd, c); }

  method fs::WriteResult write_all(mc c) noexcept {
    return fs::write_all(fd, c);
  }

  // A convenience wrapper of write_all method for clients which always
  // assume that all bytes will be written from given input without errors.
  //
  // In practice method gives up as soon as it encounters first write error.
  // No additional attempts to write the rest of input are made
  method void print(str s) noexcept { write_all(s); }

  method fs::CloseResult close() noexcept { return os::close(fd); }
};

var Sink raw_stdout = Sink(linux::stdout_fd);
var Sink raw_stderr = Sink(linux::stderr_fd);

internal const usz default_sink_buf_size = 1 << 13;

var u8 stdout_buf[default_sink_buf_size];
var u8 stderr_buf[default_sink_buf_size];

var bufio::Writer<Sink> stdout =
    bufio::Writer<Sink>(raw_stdout, mc(stdout_buf, default_sink_buf_size));
var bufio::Writer<Sink> stderr =
    bufio::Writer<Sink>(raw_stdout, mc(stdout_buf, default_sink_buf_size));

// Represents a one-way producer of byte stream. Bytes can be read
// from it, but cannot be written to
//
// This consumer is usually tied to some external resource (open file,
// socket, pipe, network connection, etc.) and thus read operation can
// result in error which is outside of current program control. From this
// perspective encountering errors when interacting with this object is
// considered normal behaviour and should not be treated by clients as
// something extraordinary and unexpected. Instead errors produced on
// reads should be checked and handled accordingly to their nature
//
// Note that this implementation does unbuffered reads. To make buffered
// version use bufio::Reader
struct Tap {
  fs::FileHandle fd;

  let Tap(fs::FileHandle fd) noexcept : fd(fd) {}

  method fs::ReadResult read(mc c) noexcept { return fs::read(fd, c); }

  method fs::ReadResult read_all(mc c) noexcept { return fs::read_all(fd, c); }
};

var Tap raw_stdin = Tap(linux::stdin_fd);

fn io::OpenResult create(str path) noexcept {

}

}  // namespace coven::os
