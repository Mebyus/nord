namespace coven::debug {

// caller should supply memory chunk of at least 256 bytes long
fn mc print_types_info(mc c) noexcept {
  var fmt::Buffer buf = fmt::Buffer(c);

  // Clumsy C++ builtin types
  buf.write(static_string("\nint\t\t\t"));
  buf.dec(sizeof(int));

  buf.write(static_string("\nlong int\t\t"));
  buf.dec(sizeof(long int));

  buf.write(static_string("\nsigned long int\t\t"));
  buf.dec(sizeof(signed long int));

  buf.write(static_string("\nunsigned int\t\t"));
  buf.dec(sizeof(unsigned int));

  buf.write(static_string("\nunsigned long int\t"));
  buf.dec(sizeof(unsigned long int));

  buf.lf();

  // Coven types
  buf.write(static_string("\nu8\t\t"));
  buf.dec(sizeof(u8));

  buf.write(static_string("\nu16\t\t"));
  buf.dec(sizeof(u16));

  buf.write(static_string("\nu32\t\t"));
  buf.dec(sizeof(u32));

  buf.write(static_string("\nu64\t\t"));
  buf.dec(sizeof(u64));

  buf.write(static_string("\nu128\t\t"));
  buf.dec(sizeof(u128));

  buf.lf();

  buf.write(static_string("\ni8\t\t"));
  buf.dec(sizeof(i8));

  buf.write(static_string("\ni16\t\t"));
  buf.dec(sizeof(i16));

  buf.write(static_string("\ni32\t\t"));
  buf.dec(sizeof(i32));

  buf.write(static_string("\ni64\t\t"));
  buf.dec(sizeof(i64));

  buf.write(static_string("\ni128\t\t"));
  buf.dec(sizeof(i128));

  buf.lf();

  buf.write(static_string("\nf32\t\t"));
  buf.dec(sizeof(f32));

  buf.write(static_string("\nf64\t\t"));
  buf.dec(sizeof(f64));

  buf.write(static_string("\nf128\t\t"));
  buf.dec(sizeof(f128));

  buf.lf();

  buf.write(static_string("\nuarch\t\t"));
  buf.dec(sizeof(uarch));

  buf.write(static_string("\niarch\t\t"));
  buf.dec(sizeof(iarch));

  buf.write(static_string("\nuptr\t\t"));
  buf.dec(sizeof(void*));

  buf.lf();

  buf.write(static_string("\nmc\t\t"));
  buf.dec(sizeof(mc));

  buf.lf();

  return buf.head();
}

}  // namespace debug