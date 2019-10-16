# The Northeastern Shell (nush)

## Usage
Just compile the code and run the executable:
```
cd nush/
make
./nush
```

## How it Works
The shell works by tokenizing user commands and parsing them to construct a syntax tree. Individual commands can be executed with a system call, whereas special characters, like redirect operators and pipes, are each implemented with their own functionality, like creating file descriptors and new processes to handle instructions.
