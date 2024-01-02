namespace fs {

fn WriteResult write(FileDescriptor fd, mc c) noexcept {
  var isz n = ::write(cast(i32, fd), c.ptr, c.len);
  if (n < 0) {
    // TODO: add error handling
    return WriteResult(WriteResult::Code::Error, n);
  }
  return WriteResult(n);
}

}  // namespace fs
