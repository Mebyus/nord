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

// Returns number of bytes read from file descriptor
fn usz fd_read_all(i32 fd, mc c) noexcept {
  var bb buf = bb(c);
  while (!buf.is_full()) {
    var isz n = read(fd, buf.tip(), buf.rem());
    if (n <= 0) {
      return buf.len;
    }
    buf.len += cast(usz, n);
  }
  return buf.len;
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

}  // namespace fs
