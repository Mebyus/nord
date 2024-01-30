namespace coven::os::linux {

// Wrapper type for raw integer value of linux file descriptor
//
// When passing around file descriptors this struct is used instead of
// raw integer types. This is mostly done as a (somewhat mouthful) technique
// to enhance poor integer type safety in C++
struct FileDescriptor {
  uarch val;

  let FileDescriptor() noexcept : val(0) {}
  let FileDescriptor(uarch val) noexcept : val(val) {}
};

internal const FileDescriptor stdin = FileDescriptor(0);
internal const FileDescriptor stdout = FileDescriptor(1);
internal const FileDescriptor stderr = FileDescriptor(2);

// Describes result of open system call in more friendly way
// than regular integer from raw syscall
struct OpenSyscallResult {
  FileDescriptor fd;

  OpenResult::Code code;

  let OpenSyscallResult(FileDescriptor fd) noexcept : fd(fd), code(OpenResult::Code::Ok) {}
  let OpenSyscallResult(OpenResult::Code code) noexcept : fd(FileDescriptor()), code(code) {}

  method bool is_ok() const noexcept { return code == OpenResult::Code::Ok; }
  method bool is_err() const noexcept { return code != OpenResult::Code::Ok; }
};

fn internal OpenResult::Code dispatch_open_error(syscall::Error err) noexcept {
  switch (err) {
  case syscall::Error::OK: {
    crash();
  }
  case syscall::Error::EACCES:
  case syscall::Error::EBUSY:
  case syscall::Error::EDQUOT:
  case syscall::Error::EEXIST:
  case syscall::Error::EFAULT:
  case syscall::Error::EFBIG:
  case syscall::Error::EINTR:
  case syscall::Error::EINVAL:
  case syscall::Error::EISDIR:
  case syscall::Error::ELOOP:
  case syscall::Error::EMFILE:
  case syscall::Error::ENAMETOOLONG:
  case syscall::Error::ENFILE:
  case syscall::Error::ENODEV:
  case syscall::Error::ENOENT:
  case syscall::Error::ENOMEM:
  case syscall::Error::ENOSPC:
  case syscall::Error::ENOTDIR:
  case syscall::Error::ENXIO:
  case syscall::Error::EOPNOTSUPP:
  case syscall::Error::EOVERFLOW:
  case syscall::Error::EPERM:
  case syscall::Error::EROFS:
  case syscall::Error::ETXTBSY:
  case syscall::Error::EWOULDBLOCK:
  default:
    return OpenResult::Code::Error;
  }
}

fn inline OpenSyscallResult open(cstr path, u32 flags, u32 mode) noexcept {
  var syscall::Result r dirty;
  do {
    r = syscall::open(path.ptr, flags, mode);
  } while (r.err == syscall::Error::EINTR);

  if (r.is_ok()) {
    return OpenSyscallResult(r.val);
  }

  return OpenSyscallResult(dispatch_open_error(r.err));
}

fn OpenSyscallResult create(cstr path) noexcept {
  const u32 flags = cast(u32, syscall::OpenFlags::O_CREAT) | 
                    cast(u32, syscall::OpenFlags::O_TRUNC) |
                    cast(u32, syscall::OpenFlags::O_WRONLY);

  return open(path, flags, 0644);
}

fn inline io::CloseResult::Code dispatch_close_error(syscall::Error err) noexcept {
  switch (err) {

  case syscall::Error::OK: {
    // must be unreachable if function is used correctly
    crash();
  }

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

fn io::CloseResult close(FileDescriptor fd) noexcept {
  var syscall::Result r dirty;
  do {
    r = syscall::close(cast(u32, fd.val));
  } while (r.err == syscall::Error::EINTR);

  if (r.is_ok()) {
    return io::CloseResult();
  }

  return io::CloseResult(dispatch_close_error(r.err));
}

fn inline io::ReadResult::Code dispatch_read_error(syscall::Error err) noexcept {
  switch (err) {
    case syscall::Error::OK: {
      crash();
    }
    case syscall::Error::EAGAIN:
    case syscall::Error::EBADF:
    case syscall::Error::EFAULT:
    case syscall::Error::EINVAL:
    case syscall::Error::EIO:
    case syscall::Error::EISDIR:
  default:
    // Unknown error code
    return io::ReadResult::Code::Error;
  }
}

fn io::ReadResult read(FileDescriptor fd, mc buf) noexcept {
  // TODO: check too large buffer size (at most 0x7ffff000)
  var syscall::Result r dirty;
  do {
    r = syscall::read(cast(u32, fd.val), buf.ptr, buf.len);
  } while (r.err == syscall::Error::EINTR);

  if (r.is_ok()) {
    if (r.val == 0) {
      return io::ReadResult(io::ReadResult::Code::EOF);
    }
    return io::ReadResult(cast(uarch, r.val));
  }

  return io::ReadResult(dispatch_read_error(r.err));
}

fn inline io::WriteResult::Code dispatch_write_error(syscall::Error err) noexcept {
  switch (err) {
    case syscall::Error::OK: {
      crash();
    }
    case syscall::Error::EAGAIN:
    case syscall::Error::EBADF:
    case syscall::Error::EFAULT:
    case syscall::Error::EINVAL:
    case syscall::Error::EIO:
    case syscall::Error::EISDIR:
  default:
    // Unknown error code
    return io::WriteResult::Code::Error;
  }
}

fn io::WriteResult write(FileDescriptor fd, mc buf) noexcept {
  // TODO: check too large buffer size (at most 0x7ffff000)
  var syscall::Result r dirty;
  do {
    r = syscall::write(cast(u32, fd.val), buf.ptr, buf.len);
  } while (r.err == syscall::Error::EINTR);

  if (r.is_ok()) {
    return io::WriteResult(cast(uarch, r.val));
  }

  return io::WriteResult(dispatch_write_error(r.err));
}

}  // namespace coven::os::linux

namespace coven::os {

fn io::ReadResult read(FileStream stream, mc c) noexcept {
  const linux::FileDescriptor fd = linux::FileDescriptor(stream.handle);
  return linux::read(fd, c);
}

fn io::WriteResult write(FileStream stream, mc c) noexcept {
  const linux::FileDescriptor fd = linux::FileDescriptor(stream.handle);
  return linux::write(fd, c);
}

fn internal inline FileStream convert_to_file_stream(linux::FileDescriptor fd) noexcept {
  return FileStream(fd.val);
}

fn internal inline OpenResult convert_to_open_result(linux::OpenSyscallResult r) noexcept {
  return OpenResult(r.code, r.fd.val);
}

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
  FileStream stream;

  let Sink(FileStream stream) noexcept : stream(stream) {}

  method io::WriteResult write(mc c) noexcept { return os::write(stream, c); }

  method io::WriteResult write_all(mc c) noexcept {
    return os::write_all(stream, c);
  }

  // A convenience wrapper of write_all method for clients which always
  // assume that all bytes will be written from given input without errors.
  //
  // In practice method gives up as soon as it encounters first write error.
  // No additional attempts to write the rest of input are made
  method void print(str s) noexcept { write_all(s); }

  method io::CloseResult close() noexcept { return os::close(stream); }
};

fn internal inline Sink convert_to_sink(linux::FileDescriptor fd) noexcept {
  return Sink(convert_to_file_stream(fd.val));
}

var Sink raw_stdout = convert_to_sink(linux::stdout);
var Sink raw_stderr = convert_to_sink(linux::stderr);

internal const uarch default_sink_buf_size = 1 << 13;

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
  FileStream stream;

  let Tap(FileStream stream) noexcept : stream(stream) {}

  method io::ReadResult read(mc c) noexcept { return os::read(stream, c); }

  method io::ReadResult read_all(mc c) noexcept { return os::read_all(stream, c); }
};

fn internal inline Tap convert_to_tap(linux::FileDescriptor fd) noexcept {
  return Tap(convert_to_file_stream(fd.val));
}

var Tap raw_stdin = convert_to_tap(linux::stdin);

fn OpenResult create(str path) noexcept {
  const uarch path_buf_length = 1 << 14;
  if (path.len >= path_buf_length) {
    return OpenResult(OpenResult::Code::PathTooLong);
  }

  var u8 buf[path_buf_length] dirty;
  var mc path_buf = mc(buf, path_buf_length);
  var cstr path_as_cstr = unsafe_copy_as_cstr(path, path_buf);

  return convert_to_open_result(linux::create(path_as_cstr));
}

// Read entire file and return its contents as raw bytes
//
// Memory allocations are performed via supplied allocator
template <typename A>
fn FileReadResult read_file(A& alc, str path) noexcept {
  
}

fn FileReadResult read_file(str path) noexcept {
  const uarch path_buf_length = 1 << 14;
  if (path.len >= path_buf_length) {
    return FileReadResult(FileReadResult::Code::PathTooLong);
  }

  var u8 buf[path_buf_length] dirty;
  var mc path_buf = mc(buf, path_buf_length);
  var cstr path_as_cstr = unsafe_copy_as_cstr(path, path_buf);

  var struct stat s dirty;
  const i32 rcode = stat(cast(char*, path.ptr), &s);
  if (rcode < 0) {
    return FileReadResult(FileReadResult::Code::Error);
  }

  const uarch size = s.st_size;
  if (size == 0) {
    // File is empty, nothing to read
    return FileReadResult(mc());
  }

  const i32 fd = ::open(cast(char*, path.ptr), O_RDONLY);
  if (fd < 0) {
    return FileReadResult(FileReadResult::Code::Error);
  }

  var mc data = mem::alloc(size);
  if (data.is_nil()) {
    close(fd);
    return FileReadResult(FileReadResult::Code::Error);
  }

  var ReadResult rr = read_all(fd, data);
  close(fd);
  if (rr.is_err() || rr.n == 0) {
    mem::free(data);
    return FileReadResult(FileReadResult::Code::Error);
  }

  return FileReadResult(data.slice_to(rr.n));
}

}  // namespace coven::os
