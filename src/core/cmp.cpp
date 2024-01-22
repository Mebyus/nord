namespace coven::cmp {

// Two memory chunks are considered equal if they have
// identical length and bytes content
fn bool equal(mc a, mc b) noexcept {
  if (a.len != b.len) {
    return false;
  }

  const uarch len = a.len;

  for (uarch i = 0; i < len; i += 1) {
    if (a.ptr[i] != b.ptr[i]) {
      return false;
    }
  }

  return true;
}

} // namespace coven::cmp
