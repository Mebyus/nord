#include <ctype.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include "core.hpp"

#include "core_linux.cpp"

namespace nord {

var global struct termios original_terminal_state;

fn internal void exit_raw_mode() noexcept {
  i32 rcode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_terminal_state);
  if (rcode < 0) {
    exit(3);
  }
}

fn internal void enter_raw_mode() noexcept {
  i32 rcode = tcgetattr(STDIN_FILENO, &original_terminal_state);
  if (rcode < 0) {
    exit(3);
  }

  struct termios terminal_state = original_terminal_state;
  terminal_state.c_cflag |= (cast(u32, CS8));
  terminal_state.c_oflag &= ~(cast(u32, OPOST));
  terminal_state.c_iflag &=
      ~(cast(u32, BRKINT) | cast(u32, INPCK) | cast(u32, ISTRIP) |
        cast(u32, ICRNL) | cast(u32, IXON));
  terminal_state.c_lflag &= ~(cast(u32, ECHO) | cast(u32, ICANON) |
                              cast(u32, IEXTEN) | cast(u32, ISIG));

  rcode = tcsetattr(STDIN_FILENO, TCSAFLUSH, &terminal_state);
  if (rcode < 0) {
    exit(3);
  }
  atexit(exit_raw_mode);
}

}  // namespace nord

fn i32 main() noexcept {
  nord::enter_raw_mode();

  var u8 input_buf[7];
  var u8 output_buf[256];
  var mc ibuf = mc(input_buf, sizeof(input_buf));
  var bb buf = bb(output_buf, sizeof(output_buf));

  const fs::FileDescriptor stdin_fd = 0;
  const fs::FileDescriptor stdout_fd = 1;

  // track number of successfully read input sequences
  var u64 k = 0;

  while (true) {
    var fs::ReadResult rr = fs::read(stdin_fd, ibuf);
    if (rr.is_err()) {
      return 1;
    }

    buf.write(macro_static_str("\r\n["));
    buf.fmt_dec(k);
    buf.write(macro_static_str("] -> "));
    buf.fmt_dec(rr.n);
    if (rr.n == 1) {
      buf.write(macro_static_str(" byte"));
    } else {
      buf.write(macro_static_str(" bytes"));
    }
    buf.write(macro_static_str("\r\n\r\n"));

    for (usz i = 0; i < rr.n; i += 1) {
      const u8 x = ibuf.ptr[i];

      fmt::unsafe_hex_prefix_byte(buf.tail(), x);
      buf.len += 4;

      buf.write(macro_static_str("  =>  "));

      buf.fmt_dec(x);

      buf.write(macro_static_str("\r\n"));
    }

    buf.write(macro_static_str("\r\n-------\r\n"));

    fs::write_all(stdout_fd, buf.head());
    buf.reset();

    if (rr.n != 0 && ibuf.ptr[0] == (('q') & 0x1f)) {
      // exit on ctrl + q
      return 0;
    }

    k += 1;
  }

  return 0;
}
