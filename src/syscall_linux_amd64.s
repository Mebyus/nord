.global coven_linux_syscall_open
.global coven_linux_syscall_exit
.global coven_linux_syscall_mmap
.global coven_linux_syscall_read

// Brief summary of syscall convetions on linux_amd64 platform
//
// 1. Syscall number is placed into rax register
//
// 2. Syscall arguments:
//
//  arg0 - rdi
//  arg1 - rsi
//  arg2 - rdx
//  arg3 - r10
//  arg4 - r8
//  arg5 - r9
//
// 3. Two registers rcx and r11 are clobbered after syscall exits
//
// 4. Result of syscall execution is placed into rax register upon
// syscall exit. Meaning of this result depends on particular syscall

// fn open(s: *u8, flags: u32, mode: u32) => i32
//
//  [s]     => rdi
//  [flags] => rsi
//  [mode]  => rdx
coven_linux_syscall_open:
    // All arguments are already set in place for syscall by function
    // calling convention
    //
    // open syscall number => 0x02
    //
    //  [s]     => arg0 => rdi
    //  [flags] => arg1 => rsi
    //  [mode]  => arg2 => rdx
    mov $0x02, %rax
    syscall
    ret

// fn exit(code: i32) => never
//
//  code => rdi
coven_linux_syscall_exit:
    // Exit syscall accepts a single argument - integer with process exit code
    //
    // The first argument to this function is passed inside rdi register,
    // thus we do not need to setup argument passing to syscall, because
    // it also accepts exit code in rdi register
    //
    // exit syscall number => 0x3C
    //
    //  [code] => arg0 => rdi 
	mov 	$0x3C, %rax 
	syscall
    // Ret instruction is not needed because exit terminates process execution

// fn mmap(addr: anyptr, len: usz, prot: i32, flags: i32, fd: i32, offset: i32) => anyptr
//
//  [addr]   => rdi
//  [len]    => rsi
//  [prot]   => rdx
//  [flags]  => rcx
//  [fd]     => r8
//  [offset] => r9
coven_linux_syscall_mmap:
    // All arguments except flags are already set in place for syscall
    // by function calling convention
    //
    // Move flags argument into syscall arg3 register (r10)
    mov %rcx, %r10

    // mmap syscall number => 0x09
    //
    //  [addr]   => arg0 => rdi
    //  [len]    => arg1 => rsi
    //  [prot]   => arg2 => rdx
    //  [flags]  => arg3 => r10
    //  [fd]     => arg4 => r8
    //  [offset] => arg5 => r9
    mov $0x09, %rax
    syscall
    ret
