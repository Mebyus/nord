fn extern "C" void coven_sync_spin(u16 cycles) noexcept;

namespace coven::sync {

fn inline void spin(u16 cycles) noexcept {
  coven_sync_spin(cycles);
}

}  // namespace coven::sync
