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

fn i32 main(i32 argc, u8** argv) noexcept {
  if (argc < 2) {
    return 1;
  }
  var cstr filename = cstr(argv[1]);

  return 0;
}
