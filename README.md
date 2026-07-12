# RISC-V RV32I Pipelined CPU Simulator

A 5-stage pipelined CPU simulator implementing the full RV32I base integer instruction set, written in C++17.

Supports both **single-cycle** and **5-stage pipelined** execution modes in one binary, selectable at runtime. The pipelined mode implements load-use hazard detection with stalls, operand forwarding (EX/MEM and MEM/WB paths), and control hazard flushing for branches and jumps.

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

**Hazard handling:**
- **Load-use hazard** → 1-cycle stall + forwarding after stall
- **Control hazard** → 2-cycle flush on taken branch/jump
- **RAW data hazard** → resolved by EX/MEM and MEM/WB forwarding paths, zero stalls

---

## Features

- Full RV32I instruction set — all 40 base integer instructions
- Two execution modes in one binary: `SingleCycle` and `Pipelined`
- 5-stage pipeline: IF → ID → EX → MEM → WB
- Four pipeline registers: IF/ID, ID/EX, EX/MEM, MEM/WB with `valid` flag bubble propagation
- `HazardDetector` class — detects load-use hazards and branch flushes
- `ForwardingUnit` class — dual forwarding paths (EX/MEM → EX, MEM/WB → EX) with correct priority
- Pipeline register snapshot mechanism to correctly model simultaneous stage execution
- 64KB byte-addressable data memory, little-endian
- Modular class-based architecture: `CPU`, `Memory`, `RegisterFile`, `ALU`, `Decoder`, `HazardDetector`, `ForwardingUnit`
- Register dump with ABI names, hex and decimal values on exit
- Cycle count reported on exit

---

## Project Structure

```
RISCVI32SimulatorPipelined/
├── main.cpp
├── makefile
├── build.bat                  # Windows quick build
├── include/
│   ├── ALU.h
│   ├── CPU.h
│   ├── Decoder.h
│   ├── ForwardingUnit.h
│   ├── HazardDetector.h
│   ├── Memory.h
│   ├── Opcodes.h
│   ├── PipelineRegs.h         # IFID, IDEX, EXMEM, MEMWB structs
│   └── RegisterFile.h
├── src/
│   ├── ALU.cpp
│   ├── CPU.cpp                # Both single-cycle and pipeline implementations
│   ├── Decoder.cpp
│   ├── ForwardingUnit.cpp
│   ├── HazardDetector.cpp
│   ├── Memory.cpp
│   └── RegisterFile.cpp
└── asm/
    ├── factorial_loop.bin
    ├── factorial_baseset.bin
    ├── fibonacci.bin
    └── load_hazard_test.bin
```

---

## Prerequisites

- C++17 compiler — `g++` 9 or later, or `clang++` 10 or later
- `make` (Linux/macOS) or MinGW64 (Windows)

No external dependencies.

---

## Build

**Linux / macOS**
```bash
make
```

**Windows — MSYS2 MinGW64 terminal**
```bash
make
```

**Windows — Command Prompt**
```
build.bat
```

Output binary: `simulator` (or `simulator.exe` on Windows).

---

## Usage

```
./simulator <program.bin> <mode>
```

`<mode>` is either `SingleCycle` or `Pipelined`.

**Examples**
```bash
./simulator asm/factorial_loop.bin SingleCycle
./simulator asm/factorial_loop.bin Pipelined
./simulator asm/fibonacci.bin Pipelined
```

The input `.bin` file is produced by the companion [RV32I Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler) — one 32-bit binary string per line, plain text.

---

## Verified Results

All programs produce identical register state in both modes. Cycle counts differ due to pipeline fill, branch penalties, and load-use stalls.

| Program | Mode | Result (`a0`) | Cycles |
|---------|------|---------------|--------|
| `factorial_loop.bin` | SingleCycle | 120 ✓ | 95 |
| `factorial_loop.bin` | Pipelined | 120 ✓ | 121 |
| `fibonacci.bin` | SingleCycle | 55 ✓ | 50 |
| `load_hazard_test.bin` | SingleCycle | — | 5 |
| `load_hazard_test.bin` | Pipelined | — | 9 |

**Pipeline overhead breakdown for `factorial_loop`:**

```
Single-cycle cycles:   95   (one cycle per instruction)
Pipeline cycles:      121
───────────────────────────
Extra cycles:          26
  Pipeline drain:       4   (last instruction takes 4 extra cycles to reach WB)
  Branch penalties:    22   (11 taken branches × 2 flush cycles each)
```

**Load-use hazard test (`load_hazard_test.bin`):**

```asm
addi x1, x0, 256     # x1 = 256
lw   x2, 0(x1)       # x2 = mem[256]   ← LOAD
add  x3, x2, x1      # x3 = x2 + x1    ← USE x2 immediately: load-use hazard
addi x4, x0, 5
ecall
```

Pipeline: 1 stall cycle inserted, result correct. 9 cycles vs 5 single-cycle (4 pipeline drain + 1 stall).

---

## Architecture

**`CPU`** — owns all subcomponents as direct members. Implements both `runSingleCycle()` and `runPipelined()`, dispatched via `startSimulation(RunMode)`. The pipeline loop runs stages in reverse order (WB→MEM→EX→ID→IF) to model simultaneous hardware execution. Pipeline registers are snapshotted before each stage to prevent overwrite corruption between stages in the same cycle.

**`HazardDetector`** — stateless class. Takes the current pipeline register state and returns a `HazardSignals` struct with `stall` and `flush` booleans. Load-use detection checks the IDEX stage for a LOAD whose `rd` matches `rs1` or `rs2` of the incoming instruction. Branch/jump flush detection checks `EXMEM.branch_taken`.

**`ForwardingUnit`** — stateless class. Takes `IDEX`, `EXMEM`, `MEMWB` snapshots and returns forwarded `rs1_val` and `rs2_val`. EX/MEM path takes priority over MEM/WB via `else if` — critical for back-to-back writes to the same register. Both paths check `valid`, `reg_write`, and `rd != 0`.

**`Decoder`** — extracts all instruction fields from a raw 32-bit encoding. Handles all six instruction formats with correct sign-extended immediates, including the scrambled bit layouts of B-type and J-type encodings.

**`Memory`** — separate instruction and data memory. Instruction memory loaded from `.bin` file at startup. 64KB data memory, byte-addressable, little-endian. Exposes word/halfword/byte load and store operations.

**`ALU`** — stateless. Returns `ALUResult` with computed value and zero flag. Handles signed/unsigned distinction for SLT/SLTU and SRA/SRL via `int32_t` cast before shifting.

**`RegisterFile`** — 32 × 32-bit registers. Silently ignores writes to `x0`. Exposes a formatted `dump()` with ABI names in hex and decimal.

---

## Implementation Notes

**Pipeline register snapshots** — the key challenge in a sequential pipeline simulator is that running stages in order causes each stage to overwrite shared pipeline registers before downstream stages read them. This simulator takes a snapshot of `EXMEM` and `MEMWB` at the start of each cycle and passes them to `stageEX` and the forwarding unit, correctly modelling the simultaneous clock-edge behaviour of real hardware.

**Bubble propagation** — instead of inserting a real NOP encoding, each pipeline register carries a `bool valid` flag. When `valid = false`, every stage treats the register as a bubble and produces no side effects. Stalls set `exmem.valid = false`. Flushes set `ifid.valid = false` and `idex.valid = false`.

**ECALL termination** — detected in `stageID`. Sets `halted = true` and marks both `ifid` and `idex` as invalid, stopping new fetches while the pipeline drains. The main loop continues until all pipeline registers are invalid.

---

## Related

- **Phase 1 — RV32I Assembler:** [notshauryachauhan/RISCVI32Assembler](https://github.com/notshauryachauhan/RISCVI32Assembler)
- **Phase 2 — Single-Cycle Simulator:** [notshauryachauhan/RISCVI32Simulator](https://github.com/notshauryachauhan/RISCVI32SimulatorSingleCycle)

---

## Author

Shaurya Chauhan — EE, IIT Bhubaneswar  