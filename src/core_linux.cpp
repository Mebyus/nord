namespace coven::time {

fn u64 clock() noexcept {
  var u32 hi;
  var u32 lo;
  var u32 id;

  asm(R"(
    rdtscp
  )"
      : "=d"(hi), "=a"(lo), "=c"(id));

  // TODO: optimze weird mov %eax,%eax instruction in compiler output
  var u64 r = hi;
  r <<= 32;
  r |= lo;
  return r;
}

}  // namespace coven::time

fn void copy(u8* src, u8* dst, usz n) noexcept {
  must(n != 0);
  must(src != dst);

  memcpy(dst, src, n);
}

fn void move(u8* src, u8* dst, usz n) noexcept {
  must(n != 0);
  must(src != dst);

  memmove(dst, src, n);
}

method void mc::clear() noexcept {
  must(len != 0);
  must(ptr != nil);

  memset(ptr, 0, len);
}

fn void fd_write(usz fd, mc c) noexcept {
  var isz n = write(cast(i32, fd), c.ptr, c.len);
  if (n < 0) {
    // TODO: add error handling
    return;
  }
}

fn void stdout_write(mc c) noexcept {
  const i32 stdout_fd = 1;
  fd_write(stdout_fd, c);
}

fn void stderr_write(mc c) noexcept {
  const i32 stderr_fd = 2;
  fd_write(stderr_fd, c);
}

fn void fd_write_all(i32 fd, mc c) noexcept {
  var u8* ptr = c.ptr;
  var usz len = c.len;
  while (len > 0) {
    var isz n = write(fd, ptr, len);
    if (n < 0) {
      // TODO: add error handling
      return;
    }

    len -= cast(usz, n);
    ptr += n;
  }
}

fn void stdout_write_all(mc c) noexcept {
  const i32 stdout_fd = 1;
  fd_write_all(stdout_fd, c);
}

fn void stderr_write_all(mc c) noexcept {
  const i32 stderr_fd = 2;
  fd_write_all(stderr_fd, c);
}

namespace fs {

fn OpenResult open(str filename) noexcept {
  const usz path_buf_length = 1 << 12;
  if (filename.len >= path_buf_length) {
    return OpenResult(OpenResult::Code::PathTooLong);
  }

  var u8 path_buf[path_buf_length] dirty;
  var mc path_mc = mc(path_buf, filename.len + 1);
  var cstr path = unsafe_copy_as_cstr(filename, path_mc);

  const i32 fd = ::open(cast(char*, path.ptr), O_RDONLY);
  if (fd < 0) {
    return OpenResult(OpenResult::Code::Error);
  }

  return OpenResult(cast(FileDescriptor, fd));
}

fn OpenResult create(str filename) noexcept {
  const usz path_buf_length = 1 << 12;
  if (filename.len >= path_buf_length) {
    return OpenResult(OpenResult::Code::PathTooLong);
  }

  var u8 path_buf[path_buf_length] dirty;
  var mc path_mc = mc(path_buf, filename.len + 1);
  var cstr path = unsafe_copy_as_cstr(filename, path_mc);

  const i32 fd =
      ::open(cast(char*, path.ptr), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    return OpenResult(OpenResult::Code::Error);
  }

  return OpenResult(cast(FileDescriptor, fd));
}

fn CloseResult close(FileDescriptor fd) noexcept {
  ::close(cast(i32, fd));
  return CloseResult();
}

fn ReadResult read(FileDescriptor fd, mc c) noexcept {
  var isz n = ::read(cast(i32, fd), c.ptr, c.len);
  if (n < 0) {
    // TODO: add error handling
    return ReadResult(ReadResult::Code::Error);
  }
  if (n == 0) {
    return ReadResult(ReadResult::Code::EOF);
  }
  return ReadResult(cast(usz, n));
}

fn WriteResult write(FileDescriptor fd, mc c) noexcept {
  var isz n = ::write(cast(i32, fd), c.ptr, c.len);
  if (n < 0) {
    // TODO: add error handling
    return WriteResult(WriteResult::Code::Error);
  }
  return WriteResult(cast(usz, n));
}

fn FileReadResult read_file(str filename) noexcept {
  const usz path_buf_length = 1 << 12;
  if (filename.len >= path_buf_length) {
    return FileReadResult(FileReadResult::Code::PathTooLong);
  }

  var u8 path_buf[path_buf_length] dirty;
  var mc path_mc = mc(path_buf, filename.len + 1);
  var cstr path = unsafe_copy_as_cstr(filename, path_mc);

  var struct stat s dirty;
  const i32 rcode = stat(cast(char*, path.ptr), &s);
  if (rcode < 0) {
    return FileReadResult(FileReadResult::Code::Error);
  }

  const usz size = s.st_size;
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

}  // namespace fs

namespace os::linux::syscall {

extern "C" [[noreturn]] fn void coven_linux_syscall_exit(i32 code) noexcept;

extern "C" fn anyptr coven_linux_syscall_mmap(uptr addr,
                                              usz len,
                                              i32 prot,
                                              i32 flags,
                                              i32 fd,
                                              i32 offset) noexcept;

extern "C" fn i32 coven_linux_syscall_munmap(uptr addr, usz len) noexcept;

// First argument must be a null-terminated string with filename
extern "C" fn i32 coven_linux_syscall_open(const u8* s,
                                           u32 flags,
                                           u32 mode) noexcept;

extern "C" fn i32 coven_linux_syscall_read(u32 fd, u8* buf, usz len) noexcept;

extern "C" fn i32 coven_linux_syscall_write(u32 fd,
                                            const u8* buf,
                                            usz len) noexcept;

extern "C" fn i32 coven_linux_syscall_close(u32 fd) noexcept;

struct KernelTimespec {
  i64 sec;
  i64 nano;
};

extern "C" fn i32 coven_linux_syscall_futex(u32* addr,
                                            i32 op,
                                            u32 val,
                                            const KernelTimespec* timeout,
                                            u32* addr2,
                                            u32 val3) noexcept;

struct CloneArgs {
  // Flags bit mask
  u64 flags;

  // Where to store PID file descriptor (pid_t *)
  u64 pidfd;

  // Where to store child TID, in child's memory (pid_t *)
  u64 child_tid;

  // Where to store child TID, in parent's memory (int *)
  u64 parent_tid;

  // Signal to deliver to parent on child termination
  u64 exit_signal;

  // Pointer to lowest byte of stack
  u64 stack;

  // Size of stack
  u64 stack_size;

  // Location of new TLS
  u64 tls;

  // Pointer to a pid_t array (since Linux 5.5)
  u64 set_tid;

  // Number of elements in set_tid (since Linux 5.5)
  u64 set_tid_size;

  // File descriptor for target cgroup of child (since Linux 5.7)
  u64 cgroup;
};

extern "C" fn i32 coven_linux_syscall_clone3(CloneArgs* args) noexcept;

fn inline i32 close(u32 fd) noexcept {
  return coven_linux_syscall_close(fd);
}

}  // namespace os::linux::syscall

namespace os::linux {

internal const u32 stdin_fd = 0;
internal const u32 stdout_fd = 1;
internal const u32 stderr_fd = 2;

}  // namespace os::linux

namespace os {

[[noreturn]] fn inline void crash() noexcept {
  // Trap outputs special reserved instruction in machine code.
  // Upon executing this instruction a cpu exception is generated
  __builtin_trap();
  __builtin_unreachable();
}

fn fs::CloseResult close(fs::FileDescriptor fd) noexcept {
  linux::syscall::close(cast(u32, fd));
  return fs::CloseResult();
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
  fs::FileDescriptor fd;

  let Sink(fs::FileDescriptor fd) noexcept : fd(fd) {}

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
  fs::FileDescriptor fd;

  let Tap(fs::FileDescriptor fd) noexcept : fd(fd) {}

  method fs::ReadResult read(mc c) noexcept { return fs::read(fd, c); }

  method fs::ReadResult read_all(mc c) noexcept { return fs::read_all(fd, c); }
};

var Tap raw_stdin = Tap(linux::stdin_fd);

}  // namespace os
