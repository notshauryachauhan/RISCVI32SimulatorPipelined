# RISC-V RV32I Pipelined CPU Simulator

A 5-stage pipelined CPU simulator implementing the full RV32I base integer instruction set, written in C++17.

Supports both **Single-Cycle** and **5-Stage Pipelined** execution modes in a single binary, selectable at runtime. The pipelined mode implements load-use hazard detection with automatic bubble stalls, dual-path operand forwarding (EX/MEM and MEM/WB paths with priority resolution), and control hazard flushing for branches and jumps.

---

## Pipeline Architecture

```
   ┌────┐      ┌─────┐     ┌────┐      ┌─────┐     ┌────┐
   │ IF │────▶│IF/ID│────▶│ ID │────▶│ID/EX│────▶│ EX │
   └────┘      └─────┘     └────┘      └─────┘     └────┘
                                                     │
                                               ┌─────────┐
                                               │  Hazard │
                                               │ Detector│
                                               └─────────┘
                                                     │
   ┌────┐     ┌──────┐     ┌─────┐     ┌──────┐      │
   │ WB │◀───│MEM/WB│◀────│ MEM │◀───│EX/MEM│◀───┘
   └────┘     └──────┘     └─────┘     └──────┘
                                             │
                                       ┌──────────┐
                                       │Forwarding│
                                       │  Unit    │
                                       └──────────┘
```

**Hazard Handling Mechanisms:**
- **RAW Data Hazards** → Resolved without stalling via EX/MEM and MEM/WB forwarding paths.
- **Load-Use Data Hazards** → 1-cycle stall with bubble insertion, followed by MEM/WB operand forwarding.
- **Control Hazards** → 2-cycle flush of IF/ID and ID/EX stages on taken branches and jumps (`JAL`/`JALR`).

---

## Features

- **Full RV32I Base Integer ISA Support**:
  - **R-Type (10)**: `ADD`, `SUB`, `SLL`, `SLT`, `SLTU`, `XOR`, `SRL`, `SRA`, `OR`, `AND`
  - **I-Type Arithmetic (9)**: `ADDI`, `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI`
  - **I-Type Loads (5)**: `LB`, `LH`, `LW`, `LBU`, `LHU`
  - **I-Type Control & System (3)**: `JALR`, `ECALL`, `EBREAK`, `FENCE`
  - **S-Type Stores (3)**: `SB`, `SH`, `SW`
  - **B-Type Branches (6)**: `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`
  - **U-Type Immediates (2)**: `LUI`, `AUIPC`
  - **J-Type Unconditional Jump (1)**: `JAL`
- **Dual Execution Modes**: `SingleCycle` and `Pipelined` in one unified binary.
- **Clock-Edge Snapshotting**: Models simultaneous pipeline stage execution without stage-overwrite corruption.
- **Bubble Propagation**: Pipeline register `valid` flags propagate pipeline bubbles cleanly without artificial NOP injections.
- **Memory System**: 64KB byte-addressable, little-endian data memory supporting byte, halfword, and word operations.
- **Register File**: 32 standard 32-bit registers with hardwired `x0 = 0` and formatted register dump reporting ABI names, hexadecimal, and decimal values.

---

## Project Structure

```
RISCVI32SimulatorPipelined/
├── main.cpp                   # CLI entry point
├── makefile                   # Build configuration (Linux/macOS/MinGW)
├── build.bat                  # Windows batch build script
├── include/
│   ├── ALU.h                  # ALU declarations & operations
│   ├── CPU.h                  # Top-level CPU core & pipeline controller
│   ├── Decoder.h              # Instruction decoder & immediate sign extension
│   ├── ForwardingUnit.h       # Dual-path operand forwarding unit
│   ├── HazardDetector.h       # Load-use stall & branch flush detector
│   ├── Memory.h               # Instruction & 64KB byte-addressable data memory
│   ├── Opcodes.h              # RV32I opcode constants
│   ├── PipelineRegs.h         # IF/ID, ID/EX, EX/MEM, MEM/WB registers
│   └── RegisterFile.h         # 32-register storage with ABI formatting
├── src/
│   ├── ALU.cpp                # ALU execution logic
│   ├── CPU.cpp                # Single-cycle & pipelined execution stages
│   ├── Decoder.cpp            # Bitfield extraction & sign extension
│   ├── ForwardingUnit.cpp     # Forwarding priority resolution
│   ├── HazardDetector.cpp     # Hazard detection logic
│   ├── Memory.cpp             # Memory operations (LB/LH/LW/SB/SH/SW)
│   └── RegisterFile.cpp       # Register reads, writes, and dumps
└── asm/
    ├── factorial_loop.bin     # Factorial calculation using loops
    ├── factorial_baseset.bin  # Factorial implementation using base instructions
    ├── fibonacci.bin          # Iterative Fibonacci computation
    ├── load_hazard_test.bin   # Targeted load-use hazard verification
    └── test.bin               # Memory store, load, and branch verification
```

---

## Prerequisites

- **C++17 Compiler**: `g++` 9+ or `clang++` 10+
- **Build Tools**: `make` (Linux/macOS/MSYS2) or standard Windows Command Prompt / PowerShell

---

## Build

**Linux / macOS / MSYS2:**
```bash
make
```

**Windows (Command Prompt / PowerShell):**
```bat
build.bat
```
*(Or compile directly with `g++ -std=c++17 -Wall -Wextra -Iinclude main.cpp src/*.cpp -o simulator.exe`)*

---

## Usage

```bash
./simulator <program.bin> <mode>
```

- `<mode>`: Either `SingleCycle` or `Pipelined`.

**Examples:**
```bash
./simulator asm/factorial_loop.bin SingleCycle
./simulator asm/factorial_loop.bin Pipelined
./simulator asm/fibonacci.bin Pipelined
./simulator asm/load_hazard_test.bin Pipelined
```

The input `.bin` file is formatted as one 32-bit binary instruction per line (generated by the companion [RV32I Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler)).

---

## Verified Results

Both execution modes produce 100% identical final register file states. Cycle count variations reflect pipeline fills, branch flushes, and load-use stall cycles.

| Program | Mode | Result (`a0`) | Cycles | Description |
|---|---|---|---|---|
| `factorial_loop.bin` | SingleCycle | `120` (0x78) ✓ | 95 | Iterative 5! factorial loop |
| `factorial_loop.bin` | Pipelined | `120` (0x78) ✓ | 121 | 11 taken branches × 2 flush penalty cycles + 4 drain cycles |
| `factorial_baseset.bin` | SingleCycle | `120` (0x78) ✓ | 151 | Factorial using base ALU instruction subset |
| `factorial_baseset.bin` | Pipelined | `120` (0x78) ✓ | 190 | Branch flush penalties + pipeline latency |
| `fibonacci.bin` | SingleCycle | `55` (0x37) ✓ | 50 | 10th Fibonacci number computation |
| `fibonacci.bin` | Pipelined | `55` (0x37) ✓ | 62 | Data forwarding on loop variables + branch penalties |
| `load_hazard_test.bin` | SingleCycle | — | 5 | Consecutive load followed by ALU use |
| `load_hazard_test.bin` | Pipelined | — | 9 | 1 stall cycle inserted + 4 pipeline drain cycles |
| `test.bin` | SingleCycle | `1` (0x01) ✓ | 6 | Memory store-load consistency & branch |
| `test.bin` | Pipelined | `1` (0x01) ✓ | 12 | Load forwarding and taken branch flushing |
| `comprehensive_test.bin` | SingleCycle | `0xCAFE0009` ✓ | 342 | Full RV32I ISA, hazards, memory, & checksum stress test |
| `comprehensive_test.bin` | Pipelined | `0xCAFE0009` ✓ | 409 | 9/9 sections passed, 0 failures, verified checksum 888 |

---

## Pipeline Execution Details

1. **Cycle Snapshotting**: At the beginning of each cycle, downstream pipeline registers (`EX/MEM` and `MEM/WB`) are snapshotted so execution in `EX` and forwarding logic read inputs as of the rising clock edge.
2. **Reverse Stage Execution**: Stages execute in reverse order (`WB → MEM → EX → ID → IF`) within each cycle to model concurrent hardware operation in a sequential software loop.
3. **Forwarding Unit Priority**: Forwarding from `EX/MEM` takes priority over `MEM/WB` to ensure the most recent register update is always supplied when back-to-back instructions write to the same destination register.
4. **Load-Use Stalling**: When an instruction in `ID/EX` is a load and its destination register matches any operand source in `IF/ID`, a 1-cycle stall is signaled:
   - `IF/ID` and `PC` are frozen.
   - A bubble (`valid = false`) is inserted into `ID/EX`.
   - On the next cycle, the loaded value is forwarded directly from `MEM/WB` to `EX`.
5. **Branch Flushing**: When a branch condition evaluates to true or a jump instruction executes in `EX`, the target PC is redirected and pending instructions in `IF/ID` and `ID/EX` are invalidated.

---

## Related Projects

- **Phase 1 — RV32I Assembler:** [notshauryachauhan/RISCVI32Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler)
- **Phase 2 — Single-Cycle Simulator:** [notshauryachauhan/RISCVI32SimulatorSingleCycle](https://github.com/notshauryachauhan/RISCVI32SimulatorSingleCycle)

---

## Author

Shaurya Chauhan — EE, IIT Bhubaneswar