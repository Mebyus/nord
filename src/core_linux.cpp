#include <time.h>

// namespace time
// {
//   fn void clock () {
//     var struct timespec ts;
//     clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
//   }
// } // namespace time

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
  if (len <= 16) {
    for (usz i = 0; i < len; i += 1) {
      ptr[i] = 0;
    }
    return;
  }

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

extern "C" {

[[noreturn]] fn void coven_linux_syscall_exit(i32 code) noexcept;

fn anyptr coven_linux_syscall_mmap(anyptr ptr,
                                   usz len,
                                   i32 prot,
                                   i32 flags,
                                   i32 fd,
                                   i32 offset) noexcept;

// First argument must be a null-terminated string with filename
fn i32 coven_linux_syscall_open(const u8* s, u32 flags, u32 mode) noexcept;

fn i32 coven_linux_syscall_read(u32 fd, u8* buf, usz len) noexcept;

fn i32 coven_linux_syscall_close(u32 fd) noexcept;
//
}

namespace os::linux {

[[noreturn]] fn void abort() noexcept {
  // hlt instruction can be executed by cpu only in privileged mode
  //
  // When invoked in userspace it will always produce cpu exception.
  // By default the program will then crash and produce coredump
  asm("hlt");
  __builtin_unreachable();
}

}  // namespace os::linux
