# afu_as - 16-bits x86 assembler

The final project of system programing class in providence university, computer science department.

# Supported instructions

add $imm8, %al

add $imm16, %ax

sub $imm8, %al

sub $imm16, %ax

mov $imm, %reg

mov %reg, %reg

int $imm8

# Build

```
make
```

# Simulation

```
make test_as

make floppy

make qemu
```
