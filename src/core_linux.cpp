

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

  return OpenResult(cast(FileHandle, fd));
}

fn CloseResult close(FileHandle fd) noexcept {
  ::close(cast(i32, fd));
  return CloseResult();
}

fn ReadResult read(FileHandle fd, mc c) noexcept {
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

fn WriteResult write(FileHandle fd, mc c) noexcept {
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



