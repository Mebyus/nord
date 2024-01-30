using namespace coven;

fn i32 main() noexcept {
    const uarch buf_size = 1 << 14;
    var u8 buf[buf_size] dirty;
    var mc output = debug::print_types_info(mc(buf, buf_size));
    os::stdout.print(output);
    os::stdout.flush();
    return 0;
}
