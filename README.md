# RISC-V RV32I CPU Simulator

A single-cycle CPU simulator implementing the full RV32I base integer instruction set, written in C++17.

Takes assembled binary output (one 32-bit instruction per line) as input and simulates execution, producing a full register dump and cycle count on completion.

---

## Features

- Full RV32I instruction set — all 40 base integer instructions
- Single-cycle execution model — one instruction retired per cycle
- Modular class-based architecture — `CPU`, `Memory`, `RegisterFile`, `ALU`, `Decoder`
- 64KB byte-addressable data memory, little-endian
- Termination via `ECALL`
- Register dump with ABI names, hex and decimal values
- Cycle count and final PC printed on exit
- Safety cap of 1,000,000 cycles to catch infinite loops during testing

---

## Project Structure

```
RISCV32Simulator/
├── main.cpp
├── makefile
├── build.bat               # Windows quick build (g++ direct)
├── include/
│   ├── ALU.h
│   ├── CPU.h
│   ├── Decoder.h
│   ├── Memory.h
│   ├── Opcodes.h
│   └── RegisterFile.h
├── src/
│   ├── ALU.cpp
│   ├── CPU.cpp
│   ├── Decoder.cpp
│   ├── Memory.cpp
│   └── RegisterFile.cpp
└── asm/
    ├── factorial_loop.bin
    ├── factorial_baseset.bin
    └── fibonacci.bin
```

---

## Prerequisites

- C++17 compiler — `g++` 9 or later, or `clang++` 10 or later
- `make` (Linux/macOS) or MinGW64 (Windows)

No external libraries or dependencies.

---

## Build

**Linux / macOS**
```bash
make
```

**Windows (MSYS2 MinGW64 terminal)**
```bash
make
```

**Windows (Command Prompt — using build.bat)**
```
build.bat
```

The binary is output as `simulator` (or `simulator.exe` on Windows).

---

## Usage

```
./simulator <program.bin>
```

The input file must be a plain text file with one 32-bit binary string per line — the format produced by the companion [RV32I Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler).

**Example**
```
./simulator asm/factorial_loop.bin
```

**Output**
```
====== Register File ======
x#  ABI   HEX         DECIMAL
----------------------------------
x0  zero  0x00000000  0
x1  ra    0x00000000  0
...
x10 a0    0x00000078  120
...
===========================

Final PC: 0x00000040
Total cycles executed: 107
```

---

## Input Format

The simulator reads the binary output of the companion RV32I assembler — one 32-character binary string per line, no header, no metadata:

```
00000000010100000000010100010011
00000000000100000000001010010011
...
```

Blank lines are skipped. The assembler repo is at [notshauryachauhan/RISCVI32Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler).

---

## Supported Instructions

All 40 RV32I base integer instructions across all six instruction formats:

| Format | Instructions |
|--------|-------------|
| R-type | ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND |
| I-type (arith) | ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI |
| I-type (load) | LB, LH, LW, LBU, LHU |
| S-type | SB, SH, SW |
| B-type | BEQ, BNE, BLT, BGE, BLTU, BGEU |
| U-type | LUI, AUIPC |
| J-type | JAL, JALR |
| System | ECALL |

---

## Test Programs

Three pre-assembled programs are included in `asm/`:

| File | Description | Expected result |
|------|-------------|-----------------|
| `factorial_loop.bin` | Iterative factorial — computes 5! using a loop | `a0 = 120` |
| `factorial_baseset.bin` | Recursive factorial with stack — computes 5! | `a0 = 120` |
| `fibonacci.bin` | Iterative Fibonacci — computes fib(10) | `a0 = 55` |

Run all three:
```bash
./simulator asm/factorial_loop.bin
./simulator asm/factorial_baseset.bin
./simulator asm/fibonacci.bin
```

---

## Architecture

The simulator is structured around five classes that mirror the functional units of a real processor:

**`Memory`** — holds instruction memory (loaded from the `.bin` file) and 64KB of byte-addressable data memory. Exposes word, halfword, and byte load/store operations. Little-endian byte ordering throughout.

**`RegisterFile`** — 32 × 32-bit registers. Enforces the `x0 = 0` invariant silently on every write. Exposes a `dump()` method for formatted output.

**`ALU`** — stateless arithmetic/logic unit. Takes two `uint32_t` operands and an `ALUOp` enum value, returns a result struct with value and zero flag.

**`Decoder`** — extracts all instruction fields from a raw 32-bit encoding. Returns a `DecodedInstruction` struct with opcode, register indices, funct3/funct7, and a fully sign-extended immediate assembled from the correct format-specific bit layout.

**`CPU`** — owns all subcomponents. Implements the fetch-decode-execute loop in `run()`, with PC management, a `MapToALU()` helper for converting instruction fields to ALU operations, and `printStats()` for final output.

---

## Related

- **Phase 1 — RV32I Assembler:** [notshauryachauhan/RISCVI32Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler)

---

## Author

Shaurya Chauhan — EE, IIT Bhubaneswar  