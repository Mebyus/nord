namespace coven::os::linux::syscall {

// Note that extern "C" declarations are visible in global namespace "as is"

extern "C" [[noreturn]] fn void coven_linux_syscall_exit(i32 code) noexcept;

extern "C" fn uptr coven_linux_syscall_mmap(uptr addr,
                                              uarch len,
                                              u32 prot,
                                              u32 flags,
                                              u32 fd,
                                              i32 offset) noexcept;

extern "C" fn uptr coven_linux_syscall_anon_mmap(uptr addr,
                                              uarch len,
                                              u32 prot,
                                              u32 flags) noexcept;

extern "C" fn i32 coven_linux_syscall_munmap(uptr addr, uarch len) noexcept;

// First argument must be a null-terminated string with path to file
extern "C" fn i32 coven_linux_syscall_open(const u8* path,
                                           u32 flags,
                                           u32 mode) noexcept;

extern "C" fn i32 coven_linux_syscall_read(u32 fd, u8* buf, uarch len) noexcept;

extern "C" fn i32 coven_linux_syscall_write(u32 fd,
                                            const u8* buf,
                                            uarch len) noexcept;

extern "C" fn i32 coven_linux_syscall_close(u32 fd) noexcept;

// First argument must be a null-terminated string with path to file
extern "C" fn i32 coven_linux_syscall_mkdir(const u8* path, u32 mode) noexcept;

struct Timespec {
  i64 sec;
  i64 nano;
};

struct Stat {
  // ID of device containing file
  u64 device;

  // Inode number
  u64 inode;

  // Number of hard links
  u64 num_links;

  // File type and mode
  u32 mode;

  // User ID of owner
  u32 user_id;

  // Group ID of owner
  u32 group_id;

  // This field is placed here purely for padding. It does not
  // carry any useful information
  u32 padding;

  // Device ID (if special file)
  u64 r_device;

  // Total size in bytes
  u64 size;

  // Block size for filesystem I/O
  u64 block_size;

  // Number of 512B blocks allocated
  u64 num_blocks;

  // Time of last access
  Timespec access_time;

  // Time of last modification
  Timespec mod_time;

  // Time of last status change
  Timespec status_change_time;

  // Additional reserved fields for future compatibility
  u64 reserved[3];
};

extern "C" fn i32 coven_linux_syscall_fstat(u32 fd, Stat *stat) noexcept;

extern "C" fn i32 coven_linux_syscall_futex(u32* addr,
                                            i32 op,
                                            u32 val,
                                            const Timespec* timeout,
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

enum struct Error : u32 {
  OK = 0,

  EPERM	       = 1,  // Operation not permitted
  ENOENT       = 2,  // No such file or directory
  ESRCH	       = 3,  // No such process
  EINTR        = 4,  // Interrupted system call
  EIO          = 5,  // I/O error
  ENXIO        = 6,  // No such device or address
  E2BIG        = 7,  // Argument list too long
  ENOEXEC      = 8,  // Exec format error
  EBADF        = 9,  // Bad file number
  ECHILD       = 10, // No child processes
  EAGAIN       = 11, // Try again
  ENOMEM       = 12, // Out of memory
  EACCES       = 13, // Permission denied
  EFAULT       = 14, // Bad address
  ENOTBLK      = 15, // Block device required
  EBUSY        = 16, // Device or resource busy
  EEXIST       = 17, // File exists
  EXDEV        = 18, // Cross-device link
  ENODEV       = 19, // No such device
  ENOTDIR      = 20, // Not a directory
  EISDIR       = 21, // Is a directory
  EINVAL       = 22, // Invalid argument
  ENFILE       = 23, // File table overflow
  EMFILE       = 24, // Too many open files
  ENOTTY       = 25, // Not a typewriter
  ETXTBSY      = 26, // Text file busy
  EFBIG        = 27, // File too large
  ENOSPC       = 28, // No space left on device
  ESPIPE       = 29, // Illegal seek
  EROFS        = 30, // Read-only file system
  EMLINK       = 31, // Too many links
  EPIPE        = 32, // Broken pipe
  EDOM         = 33, // Math argument out of domain of func
  ERANGE       = 34, // Math result not representable
  EDEADLK      = 35, // Resource deadlock would occur
  ENAMETOOLONG = 36, // File name too long
  ENOLCK       = 37, // No record locks available

  // This error code is special: arch syscall entry code will return
  // -ENOSYS if users try to call a syscall that doesn't exist. To keep
  // failures of syscalls that really do exist distinguishable from
  // failures due to attempts to use a nonexistent syscall, syscall
  // implementations should refrain from returning -ENOSYS
  ENOSYS = 38, // Invalid system call number

  ENOTEMPTY       = 39,      // Directory not empty
  ELOOP           = 40,      // Too many symbolic links encountered
  EWOULDBLOCK     = EAGAIN,  // Operation would block
  ENOMSG          = 42,      // No message of desired type
  EIDRM           = 43,      // Identifier removed
  ECHRNG          = 44,      // Channel number out of range
  EL2NSYNC        = 45,      // Level 2 not synchronized
  EL3HLT          = 46,      // Level 3 halted
  EL3RST          = 47,      // Level 3 reset
  ELNRNG          = 48,      // Link number out of range
  EUNATCH         = 49,      // Protocol driver not attached
  ENOCSI          = 50,      // No CSI structure available
  EL2HLT          = 51,      // Level 2 halted
  EBADE           = 52,      // Invalid exchange
  EBADR           = 53,      // Invalid request descriptor
  EXFULL          = 54,      // Exchange full
  ENOANO          = 55,      // No anode
  EBADRQC         = 56,      // Invalid request code
  EBADSLT         = 57,      // Invalid slot
  EDEADLOCK       = EDEADLK, //
  EBFONT          = 59,      // Bad font file format
  ENOSTR          = 60,      // Device not a stream
  ENODATA         = 61,      // No data available
  ETIME           = 62,      // Timer expired
  ENOSR           = 63,      // Out of streams resources
  ENONET          = 64,      // Machine is not on the network
  ENOPKG          = 65,      // Package not installed
  EREMOTE         = 66,      // Object is remote
  ENOLINK         = 67,      // Link has been severed
  EADV            = 68,      // Advertise error
  ESRMNT          = 69,      // Srmount error
  ECOMM           = 70,      // Communication error on send
  EPROTO          = 71,      // Protocol error
  EMULTIHOP       = 72,      // Multihop attempted
  EDOTDOT         = 73,      // RFS specific error
  EBADMSG         = 74,      // Not a data message
  EOVERFLOW       = 75,      // Value too large for defined data type
  ENOTUNIQ        = 76,      // Name not unique on network
  EBADFD          = 77,      // File descriptor in bad state
  EREMCHG         = 78,      // Remote address changed
  ELIBACC         = 79,      // Can not access a needed shared library
  ELIBBAD         = 80,      // Accessing a corrupted shared library
  ELIBSCN         = 81,      // .lib section in a.out corrupted
  ELIBMAX         = 82,      // Attempting to link in too many shared libraries
  ELIBEXEC        = 83,      // Cannot exec a shared library directly
  EILSEQ          = 84,      // Illegal byte sequence
  ERESTART        = 85,      // Interrupted system call should be restarted
  ESTRPIPE        = 86,      // Streams pipe error
  EUSERS          = 87,      // Too many users
  ENOTSOCK        = 88,      // Socket operation on non-socket
  EDESTADDRREQ    = 89,      // Destination address required
  EMSGSIZE        = 90,      // Message too long
  EPROTOTYPE      = 91,      // Protocol wrong type for socket
  ENOPROTOOPT     = 92,      // Protocol not available
  EPROTONOSUPPORT = 93,      // Protocol not supported
  ESOCKTNOSUPPORT = 94,      // Socket type not supported
  EOPNOTSUPP      = 95,      // Operation not supported on transport endpoint
  EPFNOSUPPORT    = 96,      // Protocol family not supported
  EAFNOSUPPORT    = 97,      // Address family not supported by protocol
  EADDRINUSE      = 98,      // Address already in use
  EADDRNOTAVAIL   = 99,      // Cannot assign requested address
  ENETDOWN        = 100,     // Network is down
  ENETUNREACH     = 101,     // Network is unreachable
  ENETRESET       = 102,     // Network dropped connection because of reset
  ECONNABORTED    = 103,     // Software caused connection abort
  ECONNRESET      = 104,     // Connection reset by peer
  ENOBUFS         = 105,     // No buffer space available
  EISCONN         = 106,     // Transport endpoint is already connected
  ENOTCONN        = 107,     // Transport endpoint is not connected
  ESHUTDOWN       = 108,     // Cannot send after transport endpoint shutdown
  ETOOMANYREFS    = 109,     // Too many references: cannot splice
  ETIMEDOUT       = 110,     // Connection timed out
  ECONNREFUSED    = 111,     // Connection refused
  EHOSTDOWN       = 112,     // Host is down
  EHOSTUNREACH    = 113,     // No route to host
  EALREADY        = 114,     // Operation already in progress
  EINPROGRESS     = 115,     // Operation now in progress
  ESTALE          = 116,     // Stale file handle
  EUCLEAN         = 117,     // Structure needs cleaning
  ENOTNAM         = 118,     // Not a XENIX named type file
  ENAVAIL         = 119,     // No XENIX semaphores available
  EISNAM          = 120,     // Is a named type file
  EREMOTEIO       = 121,     // Remote I/O error
  EDQUOT          = 122,     // Disk quota exceeded
  ENOMEDIUM       = 123,     // No medium found
  EMEDIUMTYPE     = 124,     // Wrong medium type
  ECANCELED       = 125,     // Operation Canceled
  ENOKEY          = 126,     // Required key not available
  EKEYEXPIRED     = 127,     // Key has expired
  EKEYREVOKED     = 128,     // Key has been revoked
  EKEYREJECTED    = 129,     // Key was rejected by service
  EOWNERDEAD      = 130,     // Owner died
  ENOTRECOVERABLE = 131,     // State not recoverable
  ERFKILL         = 132,     // Operation not possible due to RF-kill
  EHWPOISON       = 133,     // Memory page has hardware error
};

// Most system calls return a single integer. A negative value
// indicates an error and non-negative gives value of positive
// system call outcome (meaning depends upon specific system call)
struct Result {
  // Value of positive system call outcome. Meaning of this value
  // depends on specific system call
  u64 val;

  // Number which indicates what kind of error occured. Equals zero
  // if system call was successful
  //
  // Descriptions of constants in Error enum type are generic.
  // For detailed description of each error refer to description
  // of particular system call
  Error err;

  // Create successful result with no carried success value
  let constexpr Result() noexcept : val(0), err(Error::OK) {}

  let constexpr Result(u64 val) noexcept : val(val), err(Error::OK) {}
  let constexpr Result(Error err) noexcept : val(0), err(err) {}

  method bool is_ok() const noexcept { return err == Error::OK; }
  method bool is_err() const noexcept { return err != Error::OK; }
};

fn [[noreturn]] inline void exit(i32 code) noexcept {
  coven_linux_syscall_exit(code);

  // diagnostic crash if somehow exit syscall fails
  crash();
}

fn inline Result close(u32 fd) noexcept {
  const i32 r = coven_linux_syscall_close(fd);
  if (r == 0) {
    // close was successful
    return Result();
  }
  if (r > 0) {
    // according to linux documentation only 0
    // can be returned on success
    crash();
  }

  const Error err = cast(Error, -r);
  return Result(err);
}

enum struct OpenFlags {

      //  O_ASYNC
      //         Enable signal-driven I/O: generate a signal (SIGIO by default, but this can be changed via fcntl(2)) when input or output becomes possible  on
      //         this file descriptor.  This feature is available only for terminals, pseudoterminals, sockets, and (since Linux 2.6) pipes and FIFOs.  See fc‐
      //         ntl(2) for further details.  See also BUGS, below.

      //  O_CLOEXEC (since Linux 2.6.23)
      //         Enable the close-on-exec flag for the new file descriptor.  Specifying this flag permits a program to avoid additional fcntl(2) F_SETFD opera‐
      //         tions to set the FD_CLOEXEC flag.

      //         Note  that  the  use  of this flag is essential in some multithreaded programs, because using a separate fcntl(2) F_SETFD operation to set the
      //         FD_CLOEXEC flag does not suffice to avoid race conditions where one thread opens a file descriptor and attempts to set its close-on-exec  flag
      //         using  fcntl(2)  at  the same time as another thread does a fork(2) plus execve(2).  Depending on the order of execution, the race may lead to
      //         the file descriptor returned by open() being unintentionally leaked to the program executed by the child process created  by  fork(2).   (This
      //         kind  of  race is in principle possible for any system call that creates a file descriptor whose close-on-exec flag should be set, and various
      //         other Linux system calls provide an equivalent of the O_CLOEXEC flag to deal with this problem.)

      //  O_CREAT
      //         If pathname does not exist, create it as a regular file.

      //         The owner (user ID) of the new file is set to the effective user ID of the process.

      //         The group ownership (group ID) of the new file is set either to the effective group ID of the process (System V semantics) or to the group  ID
      //         of  the parent directory (BSD semantics).  On Linux, the behavior depends on whether the set-group-ID mode bit is set on the parent directory:
      //         if that bit is set, then BSD semantics apply; otherwise, System V semantics apply.  For some filesystems, the behavior also depends on the bs‐
      //         dgroups and sysvgroups mount options described in mount(8).

      //         The  mode  argument  specifies  the file mode bits to be applied when a new file is created.  If neither O_CREAT nor O_TMPFILE is specified in
      //         flags, then mode is ignored (and can thus be specified as 0, or simply omitted).  The mode argument must be supplied if O_CREAT  or  O_TMPFILE
      //         is specified in flags; if it is not supplied, some arbitrary bytes from the stack will be applied as the file mode.

      //         The  effective  mode  is  modified  by  the process's umask in the usual way: in the absence of a default ACL, the mode of the created file is
      //         (mode & ~umask).

      //         Note that mode applies only to future accesses of the newly created file; the open() call that creates a read-only  file  may  well  return  a
      //         read/write file descriptor.
      //        The following symbolic constants are provided for mode:

      //         S_IRWXU  00700 user (file owner) has read, write, and execute permission

      //         S_IRUSR  00400 user has read permission

      //         S_IWUSR  00200 user has write permission

      //         S_IXUSR  00100 user has execute permission

      //         S_IRWXG  00070 group has read, write, and execute permission

      //         S_IRGRP  00040 group has read permission

      //         S_IWGRP  00020 group has write permission

      //         S_IXGRP  00010 group has execute permission

      //         S_IRWXO  00007 others have read, write, and execute permission

      //         S_IROTH  00004 others have read permission

      //         S_IWOTH  00002 others have write permission

      //         S_IXOTH  00001 others have execute permission

      //         According to POSIX, the effect when other bits are set in mode is unspecified.  On Linux, the following bits are also honored in mode:

      //         S_ISUID  0004000 set-user-ID bit

      //         S_ISGID  0002000 set-group-ID bit (see inode(7)).

      //         S_ISVTX  0001000 sticky bit (see inode(7)).

      //  O_DIRECT (since Linux 2.4.10)
      //         Try  to minimize cache effects of the I/O to and from this file.  In general this will degrade performance, but it is useful in special situa‐
      //         tions, such as when applications do their own caching.  File I/O is done directly to/from user-space buffers.  The O_DIRECT flag  on  its  own
      //         makes  an  effort  to  transfer  data  synchronously, but does not give the guarantees of the O_SYNC flag that data and necessary metadata are
      //         transferred.  To guarantee synchronous I/O, O_SYNC must be used in addition to O_DIRECT.  See NOTES below for further discussion.

      //         A semantically similar (but deprecated) interface for block devices is described in raw(8).
      //  O_DIRECTORY
      //         If pathname is not a directory, cause the open to fail.  This flag was added in kernel version 2.1.126, to avoid denial-of-service problems if
      //         opendir(3) is called on a FIFO or tape device.

      //  O_DSYNC
      //         Write operations on the file will complete according to the requirements of synchronized I/O data integrity completion.

      //         By  the time write(2) (and similar) return, the output data has been transferred to the underlying hardware, along with any file metadata that
      //         would be required to retrieve that data (i.e., as though each write(2) was followed by a call to fdatasync(2)).  See NOTES below.

      //  O_EXCL Ensure that this call creates the file: if this flag is specified in conjunction with O_CREAT, and pathname already exists, then open()  fails
      //         with the error EEXIST.

      //         When these two flags are specified, symbolic links are not followed: if pathname is a symbolic link, then open() fails regardless of where the
      //         symbolic link points.

      //         In general, the behavior of O_EXCL is undefined if it is used without O_CREAT.  There is one exception: on Linux 2.6 and later, O_EXCL can  be
      //         used without O_CREAT if pathname refers to a block device.  If the block device is in use by the system (e.g., mounted), open() fails with the
      //         error EBUSY.

      //         On NFS, O_EXCL is supported only when using NFSv3 or later on kernel 2.6 or later.  In NFS environments where O_EXCL support is not  provided,
      //         programs that rely on it for performing locking tasks will contain a race condition.  Portable programs that want to perform atomic file lock‐
      //         ing using a lockfile, and need to avoid reliance on NFS support for O_EXCL, can create a unique file on the same filesystem (e.g., incorporat‐
      //         ing  hostname and PID), and use link(2) to make a link to the lockfile.  If link(2) returns 0, the lock is successful.  Otherwise, use stat(2)
      //         on the unique file to check if its link count has increased to 2, in which case the lock is also successful.

      //  O_LARGEFILE
      //         (LFS) Allow files whose sizes cannot be represented in an off_t (but can be represented in an off64_t) to be opened.  The  _LARGEFILE64_SOURCE
      //         macro  must  be  defined  (before  including any header files) in order to obtain this definition.  Setting the _FILE_OFFSET_BITS feature test
      //         macro to 64 (rather than using O_LARGEFILE) is the preferred method of accessing large files on 32-bit systems (see feature_test_macros(7)).

      
  // The argument flags must include one of the following access modes: O_RDONLY, O_WRONLY, or O_RDWR
  //
  // These request opening the file  read-only,  write-only, or read/write, respectively.

	O_RDONLY     = 0x0,
	O_WRONLY     = 0x1,
	O_RDWR       = 0x2,

  // The file is opened in append mode.  Before each write(2), the file offset is positioned at the end of the file, as if with lseek(2)
  // The modification of the file offset and the write operation are performed as a single atomic step.
  //
  // O_APPEND may lead to corrupted files on NFS filesystems if more than one process appends data to a file at once.  This is because NFS does not
  // support appending to a file, so the client kernel has to simulate it, which can't be done without a race condition.
  O_APPEND     = 0x400,

	O_ASYNC      = 0x2000,
	O_CLOEXEC    = 0x80000,
	O_CREAT      = 0x40,
	O_DIRECT     = 0x4000,
	O_DIRECTORY  = 0x10000,
	O_DSYNC      = 0x1000,
	O_EXCL       = 0x80,
	O_FSYNC      = 0x101000,
	O_LARGEFILE  = 0x0,
	O_NDELAY     = 0x800,

  // (since Linux 2.6.8)
  // Do not update the file last access time (st_atime in the inode) when the file is read(2).
  //
  // This flag can be employed only if one of the following conditions is true:
  //
  //  *  The effective UID of the process matches the owner UID of the file.
  //  *  The calling process has the CAP_FOWNER capability in its user namespace and the owner UID of the file has a mapping in the namespace.
  //
  // This flag is intended for use by indexing or backup programs, where its use can significantly reduce the amount of disk activity. This flag
  // may not be effective on all filesystems. One example is NFS, where the server maintains the access time.
	O_NOATIME    = 0x40000,

	O_NOCTTY     = 0x100,
	O_NOFOLLOW   = 0x20000,
	O_NONBLOCK   = 0x800,
	O_RSYNC      = 0x101000,
	O_SYNC       = 0x101000,
	O_TRUNC      = 0x200,

};

// First argument must be a null-terminated string with filename
//
// If system call was successful Result value carries descriptor
// of opened (created) file
//
// Possible Result errors:
// 
//  EACCES
//    The  requested access to the file is not allowed, or search permission
//    is denied for one of the directories in the path prefix of pathname, or
//    the file did not exist yet and write access to the parent directory is not allowed
// 
//  EACCES
//    Where O_CREAT is specified, the protected_fifos or protected_regular
//    sysctl is enabled, the file already exists and is a FIFO or regular file,
//    the owner of the file is neither the current user nor the owner of
//    the containing directory, and the containing directory is both world- or
//    group-writable and sticky
// 
//  EBUSY
//    O_EXCL was specified in flags and pathname refers to a block device
//    that is in use by the system (e.g., it is mounted).
//
//  EDQUOT
//    Where O_CREAT is specified, the file does not exist, and the user's
//    quota of disk blocks or inodes on the filesystem has been exhausted
//
//        EEXIST pathname already exists and O_CREAT and O_EXCL were used.
//
//        EFAULT pathname points outside your accessible address space.
//
//        EFBIG  See EOVERFLOW.
// 
//        EINTR  While blocked waiting to complete an open of a slow device (e.g., a FIFO; see fifo(7)), the call was interrupted by a signal handler; see sig‐
//               nal(7).
// 
//        EINVAL The filesystem does not support the O_DIRECT flag.  See NOTES for more information.
// 
//        EINVAL Invalid value in flags.
// 
//  EINVAL O_TMPFILE was specified in flags, but neither O_WRONLY nor O_RDWR was specified.
//
//  EINVAL O_CREAT  was  specified  in flags and the final component ("basename") of the new file's pathname is invalid (e.g., it contains characters not
//         permitted by the underlying filesystem).
// 
//  EINVAL The final component ("basename") of pathname is invalid (e.g., it contains characters not permitted by the underlying filesystem).
// 
//  EISDIR pathname refers to a directory and the access requested involved writing (that is, O_WRONLY or O_RDWR is set).
// 
//  EISDIR pathname refers to an existing directory, O_TMPFILE and one of O_WRONLY or O_RDWR were specified in flags, but this kernel  version  does  not
//         provide the O_TMPFILE functionality.
// 
//  ELOOP  Too many symbolic links were encountered in resolving pathname.
// 
//  ELOOP  pathname was a symbolic link, and flags specified O_NOFOLLOW but not O_PATH.
// 
//  EMFILE The per-process limit on the number of open file descriptors has been reached (see the description of RLIMIT_NOFILE in getrlimit(2)).
// 
//  ENAMETOOLONG
//         pathname was too long.
// 
//  ENFILE The system-wide limit on the total number of open files has been reached.
// 
//  ENODEV pathname refers to a device special file and no corresponding device exists.  (This is a Linux kernel bug; in this situation ENXIO must be re‐
//         turned.)
// 
//  ENOENT O_CREAT is not set and the named file does not exist.
// 
//  ENOENT A directory component in pathname does not exist or is a dangling symbolic link.
// 
//  ENOENT pathname refers to a nonexistent directory, O_TMPFILE and one of O_WRONLY or O_RDWR were specified in flags, but this kernel version does  not
//         provide the O_TMPFILE functionality.
// 
//  ENOMEM The named file is a FIFO, but memory for the FIFO buffer can't be allocated because the per-user hard limit on memory allocation for pipes has
//         been reached and the caller is not privileged; see pipe(7).
// 
//  ENOMEM Insufficient kernel memory was available.
// 
//  ENOSPC pathname was to be created but the device containing pathname has no room for the new file.
// 
//  ENOTDIR
//         A component used as a directory in pathname is not, in fact, a directory, or O_DIRECTORY was specified and pathname was not a directory.
//  ENXIO  O_NONBLOCK | O_WRONLY is set, the named file is a FIFO, and no process has the FIFO open for reading.
// 
//  ENXIO  The file is a device special file and no corresponding device exists.
// 
//  ENXIO  The file is a UNIX domain socket.
// 
//  EOPNOTSUPP
//         The filesystem containing pathname does not support O_TMPFILE.
// 
//  EOVERFLOW
//         pathname refers to a regular file that is too large to be opened.  The usual scenario here is that an application compiled on a  32-bit  plat‐
//         form  without  -D_FILE_OFFSET_BITS=64  tried to open a file whose size exceeds (1<<31)-1 bytes; see also O_LARGEFILE above.  This is the error
//         specified by POSIX.1; in kernels before 2.6.24, Linux gave the error EFBIG for this case.
// 
//  EPERM  The O_NOATIME flag was specified, but the effective user ID of the caller did not match the owner of the file and the caller  was  not  privi‐
//         leged.
// 
//  EPERM  The operation was prevented by a file seal; see fcntl(2).
// 
//  EROFS  pathname refers to a file on a read-only filesystem and write access was requested.
// 
//  ETXTBSY
//         pathname refers to an executable image which is currently being executed and write access was requested.
// 
//  ETXTBSY
//         pathname refers to a file that is currently in use as a swap file, and the O_TRUNC flag was specified.
// 
//  ETXTBSY
//         pathname refers to a file that is currently being read by the kernel (e.g., for module/firmware loading), and write access was requested.
// 
//  EWOULDBLOCK
//         The O_NONBLOCK flag was specified, and an incompatible lease was held on the file (see fcntl(2)).
// 
//  The following additional errors can occur for openat():
// 
//  EBADF  dirfd is not a valid file descriptor.
// 
//  ENOTDIR
//         pathname is a relative pathname and dirfd is a file descriptor referring to a file other than a directory.
fn inline Result open(const u8* path, u32 flags, u32 mode) noexcept {
  const i32 r = coven_linux_syscall_open(path, flags, mode);
  if (r >= 0) {
    // return valid file descriptor
    return Result(cast(u32, r));
  }

  const Error err = cast(Error, -r);
  return Result(err);
}


      //  EAGAIN The file descriptor fd refers to a file other than a socket and has been marked nonblocking (O_NONBLOCK),  and  the  read  would  block.   See
      //         open(2) for further details on the O_NONBLOCK flag.

      //  EAGAIN or EWOULDBLOCK
      //         The  file descriptor fd refers to a socket and has been marked nonblocking (O_NONBLOCK), and the read would block.  POSIX.1-2001 allows either
      //         error to be returned for this case, and does not require these constants to have the same value, so a portable application  should  check  for
      //         both possibilities.

      //  EBADF  fd is not a valid file descriptor or is not open for reading.

      //  EFAULT buf is outside your accessible address space.

      //  EINTR  The call was interrupted by a signal before any data was read; see signal(7).

      //  EINVAL fd  is  attached to an object which is unsuitable for reading; or the file was opened with the O_DIRECT flag, and either the address specified
      //         in buf, the value specified in count, or the file offset is not suitably aligned.

      //  EINVAL fd was created via a call to timerfd_create(2) and the wrong size buffer was given to read(); see timerfd_create(2) for further information.

      //  EIO    I/O error.  This will happen for example when the process is in a background process group, tries to read from its controlling  terminal,  and
      //         either it is ignoring or blocking SIGTTIN or its process group is orphaned.  It may also occur when there is a low-level I/O error while read‐
      //         ing from a disk or tape.  A further possible cause of EIO on networked filesystems is when an advisory lock had been taken out on the file de‐
      //         scriptor and this lock has been lost.  See the Lost locks section of fcntl(2) for further details.

      //  EISDIR fd refers to a directory.

      //  Other errors may occur, depending on the object connected to fd.
fn inline Result read(u32 fd, u8* buf, uarch len) noexcept {
  const i32 r = coven_linux_syscall_read(fd, buf, len);
  if (r >= 0) {
    // return number of bytes read
    return Result(cast(u32, r));
  }

  const Error err = cast(Error, -r);
  return Result(err);
}

fn inline Result write(u32 fd, const u8* buf, uarch len) noexcept {
  const i32 r = coven_linux_syscall_write(fd, buf, len);
  if (r >= 0) {
    // return number of bytes written
    return Result(cast(u32, r));
  }

  const Error err = cast(Error, -r);
  return Result(err);
}

//  EACCES The  parent  directory  does not allow write permission to the process, or one of the directories in pathname did not allow search permission.
//         (See also path_resolution(7).)
//  EDQUOT The user's quota of disk blocks or inodes on the filesystem has been exhausted.
//  EEXIST pathname already exists (not necessarily as a directory).  This includes the case where pathname is a symbolic link, dangling or not.
//  EFAULT pathname points outside your accessible address space.
//  EINVAL The final component ("basename") of the new directory's pathname is invalid (e.g., it contains characters  not  permitted  by  the  underlying
//         filesystem).
//  ELOOP  Too many symbolic links were encountered in resolving pathname.
//  EMLINK The number of links to the parent directory would exceed LINK_MAX.
//  ENAMETOOLONG
//         pathname was too long.
//  ENOENT A directory component in pathname does not exist or is a dangling symbolic link.
//  ENOMEM Insufficient kernel memory was available.
//  ENOSPC The device containing pathname has no room for the new directory.
//  ENOSPC The new directory cannot be created because the user's disk quota is exhausted.
//  ENOTDIR
//         A component used as a directory in pathname is not, in fact, a directory.
//  EPERM  The filesystem containing pathname does not support the creation of directories.
//  EROFS  pathname refers to a file on a read-only filesystem.
//  The following additional errors can occur for mkdirat():
//  EBADF  dirfd is not a valid file descriptor.
//  ENOTDIR
//         pathname is relative and dirfd is a file descriptor referring to a file other than a directory.
fn inline Result mkdir(const u8* path, u32 mode) noexcept {
  const i32 r = coven_linux_syscall_mkdir(path, mode);
  if (r == 0) {
    // mkdir was successful
    return Result();
  }
  if (r > 0) {
    // according to linux documentation only 0
    // can be returned on success
    crash();
  }

  const Error err = cast(Error, -r);
  return Result(err);
}

// mmap errors
//  EACCES A file descriptor refers to a non-regular file.  Or a file mapping was requested, but fd is not open for reading.  Or MAP_SHARED was requested
//         and PROT_WRITE is set, but fd is not open in read/write (O_RDWR) mode.  Or PROT_WRITE is set, but the file is append-only.

//  EAGAIN The file has been locked, or too much memory has been locked (see setrlimit(2)).

//  EBADF  fd is not a valid file descriptor (and MAP_ANONYMOUS was not set).

//  EEXIST MAP_FIXED_NOREPLACE was specified in flags, and the range covered by addr and length clashes with an existing mapping.

//  EINVAL We don't like addr, length, or offset (e.g., they are too large, or not aligned on a page boundary).

//  EINVAL (since Linux 2.6.12) length was 0.

//  EINVAL flags contained none of MAP_PRIVATE, MAP_SHARED or MAP_SHARED_VALIDATE.

//  ENFILE The system-wide limit on the total number of open files has been reached.

//  ENODEV The underlying filesystem of the specified file does not support memory mapping.

//  ENOMEM No memory is available.

//  ENOMEM The  process's  maximum  number  of mappings would have been exceeded.  This error can also occur for munmap(), when unmapping a region in the
//         middle of an existing mapping, since this results in two smaller mappings on either side of the region being unmapped.

//  ENOMEM (since Linux 4.7) The process's RLIMIT_DATA limit, described in getrlimit(2), would have been exceeded.

//  EOVERFLOW
//         On 32-bit architecture together with the large file extension (i.e., using 64-bit off_t): the number of pages used for length plus  number  of
//         pages used for offset would overflow unsigned long (32 bits).

//  EPERM  The prot argument asks for PROT_EXEC but the mapped area belongs to a file on a filesystem that was mounted no-exec.

//  EPERM  The operation was prevented by a file seal; see fcntl(2).

//  ETXTBSY
//         MAP_DENYWRITE was set but the object specified by fd is open for writing.


const u32 PROT_READ = 0x1;
const u32 PROT_WRITE = 0x2;

const u32 MAP_SHARED = 0x01;
const u32 MAP_ANONYMOUS = 0x20;

fn inline Result anon_mmap(uptr addr, uarch len, u32 prot, u32 flags) noexcept {
  const uptr r = coven_linux_syscall_anon_mmap(addr, len, prot, flags | MAP_ANONYMOUS);

  const iarch error_check = -cast(iarch, r);
  if (0 < error_check && error_check < 256) {
    return Result(cast(Error, error_check));
  }

  return Result(r);
}

}  // namespace coven::os::linux::syscall
