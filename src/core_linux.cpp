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

fn FileReadResult read_file(cstr filename) noexcept {
  var dirty struct stat s;
  var i32 rcode = stat(cast(char*, filename.ptr), &s);
  if (rcode < 0) {
    if (errno == EEXIST) {
      return FileReadResult(FileReadResult::Code::AlreadyExists);
    }
    return FileReadResult(FileReadResult::Code::Error);
  }

  const usz size = s.st_size;
  if (size == 0) {
    // File is empty, nothing to read
    return FileReadResult(mc());
  }

  const i32 fd = open(cast(char*, filename.ptr), O_RDONLY);
  if (fd < 0) {
    if (errno == EEXIST) {
      return FileReadResult(FileReadResult::Code::AlreadyExists);
    }
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

fn i32 coven_linux_syscall_open(u8* s, u32 flags1, u32 flags2) noexcept;

fn i32 coven_linux_syscall_read() noexcept;

fn i32 coven_linux_syscall_close(i32 fd) noexcept;
}

namespace os::linux {}  // namespace os::linux
