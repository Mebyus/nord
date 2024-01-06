#include <time.h>

// namespace time
// {
//   fn void clock () {
//     var struct timespec ts;
//     clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
//   }
// } // namespace time

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
