# The Northeastern Memory Allocator (numalloc)
A multi-arena bucket allocator, optimized for consecutive small allocations.

## Usage
To use this memory allocator, please include it in one of your projects.


Rather than modifying the `malloc` syscall, this library defines a new function, `xmalloc`, with the same signature. 

## How it Works
This allocator was designed to compete with GNU's default `malloc` function on test input assuming certain and consistent parameters. The memory allocator was designed with a multi-bucket freelist, ranging the powers of 2 up to the twelfth. To reduce lock contention further, we created multiple free lists and a separate lock per bucket.
