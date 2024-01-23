namespace coven::time {

fn u64 clock() noexcept {
  var u32 hi;
  var u32 lo;
  var u32 id;

  asm(R"(
    rdtscp
  )"
      : "=d"(hi), "=a"(lo), "=c"(id));

  // TODO: optimze weird mov %eax,%eax instruction in compiler output
  var u64 r = hi;
  r <<= 32;
  r |= lo;
  return r;
}

}  // namespace coven::time
