namespace coven::os {

// Allocates (maps) at least n bytes of virtual memory in whole pages
//
// Length of returned memory chunk is always a multiple of
// memory page size and not smaller than argument n. Memory
// is requested directly from OS, that is each call to this
// function will translate to system call without any internal
// buffering
//
// Returns nil chunk if OS returns an error while allocating
// a requested amount of memory
fn mc alloc_pages(uarch n) noexcept;

// Free (unmap) memory chunk that was received from alloc_pages
// function
fn void free_pages(mc c) noexcept;

// Create with default permissions or truncate and open existing
// regular file
//
// For implementation look into source file dedicated to specific OS
fn io::OpenResult create(str path) noexcept;

} // namespace coven::os
