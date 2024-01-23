namespace os::linux::syscall {

extern "C" [[noreturn]] fn void coven_linux_syscall_exit(i32 code) noexcept;

extern "C" fn anyptr coven_linux_syscall_mmap(uptr addr,
                                              usz len,
                                              i32 prot,
                                              i32 flags,
                                              i32 fd,
                                              i32 offset) noexcept;

extern "C" fn i32 coven_linux_syscall_munmap(uptr addr, usz len) noexcept;

// First argument must be a null-terminated string with filename
extern "C" fn i32 coven_linux_syscall_open(const u8* s,
                                           u32 flags,
                                           u32 mode) noexcept;

extern "C" fn i32 coven_linux_syscall_read(u32 fd, u8* buf, usz len) noexcept;

extern "C" fn i32 coven_linux_syscall_write(u32 fd,
                                            const u8* buf,
                                            usz len) noexcept;

extern "C" fn i32 coven_linux_syscall_close(u32 fd) noexcept;

struct KernelTimespec {
  i64 sec;
  i64 nano;
};

extern "C" fn i32 coven_linux_syscall_futex(u32* addr,
                                            i32 op,
                                            u32 val,
                                            const KernelTimespec* timeout,
                                            u32* addr2,
                                            u32 val3) noexcept;

struct CloneArgs {
  // Flags bit mask
  u64 flags;

  // Where to store PID file descriptor (pid_t *)
  u64 pidfd;

  // Where to store child TID, in child's memory (pid_t *)
  u64 child_tid;

  // Where to store child TID, in parent's memory (int *)
  u64 parent_tid;

  // Signal to deliver to parent on child termination
  u64 exit_signal;

  // Pointer to lowest byte of stack
  u64 stack;

  // Size of stack
  u64 stack_size;

  // Location of new TLS
  u64 tls;

  // Pointer to a pid_t array (since Linux 5.5)
  u64 set_tid;

  // Number of elements in set_tid (since Linux 5.5)
  u64 set_tid_size;

  // File descriptor for target cgroup of child (since Linux 5.7)
  u64 cgroup;
};

extern "C" fn i32 coven_linux_syscall_clone3(CloneArgs* args) noexcept;

fn inline i32 close(u32 fd) noexcept {
  return coven_linux_syscall_close(fd);
}

}  // namespace os::linux::syscall
