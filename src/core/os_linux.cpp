namespace coven::os::linux {

internal const u32 stdin_fd = 0;
internal const u32 stdout_fd = 1;
internal const u32 stderr_fd = 2;

}  // namespace coven::os::linux

namespace coven::os {

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

}  // namespace coven::os
