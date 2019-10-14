# The Northeastern Memory Allocator (numalloc)
A multi-arena bucket allocator, optimized for consecutive small allocations.

## Usage
To use this memory allocator, please include it in one of your projects.


Rather than modifying the `malloc` syscall, this library defines a new function, `xmalloc`, with the same signature. 
