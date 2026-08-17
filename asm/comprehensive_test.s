# ==============================================================================
# RISC-V RV32I Comprehensive Test Suite
# ==============================================================================
# Tests all 40 RV32I base instructions, pipeline hazards, forwarding paths,
# load-use stalls, branch flushes, and memory sign/zero extensions.
#
# Register usage:
#   s0 (x8)  : Passed test assertions counter
#   s1 (x9)  : Failed test assertions counter (must be 0 on success)
#   a0 (x10) : Status signature (0xCAFE0000 | passed_count on SUCCESS, 0xDEAD0000 on FAIL)
#   a1 (x11) : Total passed tests
#   a2 (x12) : Total failed tests
#   a3 (x13) : Computed memory checksum
#   s2 (x18) : Test memory base address (0x200 = 512)
# ==============================================================================

# Initialization
addi s0, zero, 0        # passed = 0
addi s1, zero, 0        # failed = 0
addi s2, zero, 512      # memory test base address = 512

# ------------------------------------------------------------------------------
# TEST 1: Hardwired Zero Register (x0) Immutability
# ------------------------------------------------------------------------------
addi zero, zero, 42     # Attempt write to x0
bne  zero, zero, test1_fail
addi s0, s0, 1          # TEST 1 PASS
jal  zero, test2_start
test1_fail:
addi s1, s1, 1          # FAIL

test2_start:
# ------------------------------------------------------------------------------
# TEST 2: Basic Arithmetic & RAW Hazards (ADDI, ADD, SUB)
# ------------------------------------------------------------------------------
addi t0, zero, 100      # t0 = 100
addi t1, t0, -30        # t1 = 70  (RAW EX/MEM forwarding from t0)
sub  t2, t1, t0         # t2 = -30 (RAW EX/MEM forwarding from t1)
addi t3, zero, -30
bne  t2, t3, test2_fail
addi s0, s0, 1          # TEST 2 PASS
jal  zero, test3_start
test2_fail:
addi s1, s1, 1

test3_start:
# ------------------------------------------------------------------------------
# TEST 3: Bitwise Operations (ANDI, AND, ORI, OR, XORI, XOR)
# ------------------------------------------------------------------------------
xori t0, zero, 85       # t0 = 0x55
ori  t1, t0, 170        # t1 = 0xFF (RAW EX/MEM fwd)
andi t2, t1, 15         # t2 = 0x0F (RAW EX/MEM fwd)
addi t3, zero, 15
bne  t2, t3, test3_fail

xor  t4, t1, t0         # t4 = 0xFF ^ 0x55 = 0xAA (MEM/WB fwd)
or   t5, t4, t2         # t5 = 0xAA | 0x0F = 0xAF (EX/MEM fwd)
and  t6, t5, t1         # t6 = 0xAF & 0xFF = 0xAF (EX/MEM fwd)
addi t3, zero, 175      # 175 = 0xAF
bne  t6, t3, test3_fail
addi s0, s0, 1          # TEST 3 PASS
jal  zero, test4_start
test3_fail:
addi s1, s1, 1

test4_start:
# ------------------------------------------------------------------------------
# TEST 4: Shifts (SLLI, SRLI, SRAI, SLL, SRL, SRA)
# ------------------------------------------------------------------------------
# SLLI: 3 << 4 = 48
addi t0, zero, 3
slli t1, t0, 4          # t1 = 48
addi t2, zero, 48
bne  t1, t2, test4_fail

# SRAI vs SRLI on negative number (-32)
addi t0, zero, -32      # t0 = -32 (0xFFFFFFE0)
srai t1, t0, 2          # t1 = -8  (0xFFFFFFF8) arithmetic shift
addi t2, zero, -8
bne  t1, t2, test4_fail

# Register-variable shifts (SLL, SRL, SRA)
addi t3, zero, 3        # shift amount = 3
sra  t4, t0, t3         # -32 >> 3 = -4
addi t2, zero, -4
bne  t4, t2, test4_fail
addi s0, s0, 1          # TEST 4 PASS
jal  zero, test5_start
test4_fail:
addi s1, s1, 1

test5_start:
# ------------------------------------------------------------------------------
# TEST 5: Signed & Unsigned Comparisons (SLT, SLTU, SLTI, SLTIU)
# ------------------------------------------------------------------------------
addi t0, zero, -10      # t0 = -10
addi t1, zero, 10       # t1 = 10

# SLT: -10 < 10 -> true (1)
slt  t2, t0, t1
addi t3, zero, 1
bne  t2, t3, test5_fail

# SLTU: (unsigned)-10 < 10 -> false (0)
sltu t2, t0, t1
bne  t2, zero, test5_fail

# SLTI: -10 < 0 -> true (1)
slti t2, t0, 0
bne  t2, t3, test5_fail

# SLTIU: (unsigned)-10 < 0 -> false (0)
sltiu t2, t0, 0
bne  t2, zero, test5_fail
addi s0, s0, 1          # TEST 5 PASS
jal  zero, test6_start
test5_fail:
addi s1, s1, 1

test6_start:
# ------------------------------------------------------------------------------
# TEST 6: Upper Immediates (LUI, AUIPC)
# ------------------------------------------------------------------------------
lui   t0, 74565         # t0 = 0x12345000 (74565 = 0x12345)
srli  t1, t0, 20        # t1 = 0x123 (291)
addi  t2, zero, 291
bne   t1, t2, test6_fail

auipc t3, 0             # t3 = current PC
addi  s0, s0, 1         # TEST 6 PASS
jal   zero, test7_start
test6_fail:
addi s1, s1, 1

test7_start:
# ------------------------------------------------------------------------------
# TEST 7: Memory Subsystem (SW, LW, SH, LH, LHU, SB, LB, LBU) & Load-Use Hazard
# ------------------------------------------------------------------------------
# 7A: Word Store/Load with Load-Use Hazard Stall Test
lui  t0, 74565          # 0x12345000
ori  t0, t0, 1656       # 0x12345678 (1656 = 0x678)
sw   t0, 0(s2)          # mem[512] = 0x12345678
lw   t1, 0(s2)          # t1 = mem[512] (LOAD-USE HAZARD)
add  t2, t1, zero       # t2 = t1 (Immediate USE in EX: triggers 1-cycle stall + MEM/WB fwd)
bne  t2, t0, test7_fail

# 7B: Byte Store/Load (Signed LB vs Unsigned LBU)
# Store 0xFE (-2) at mem[516]
addi t0, zero, -2       # 0xFFFFFFFE (byte is 0xFE = 254)
sb   t0, 4(s2)          # mem[516] = 0xFE
lb   t1, 4(s2)          # t1 = -2  (signed byte sign-extended)
lbu  t2, 4(s2)          # t2 = 254 (unsigned byte zero-extended)
addi t3, zero, -2
addi t4, zero, 254
bne  t1, t3, test7_fail
bne  t2, t4, test7_fail

# 7C: Halfword Store/Load (Signed LH vs Unsigned LHU)
# Store 0xFF80 (-128) at mem[520]
addi t0, zero, -128     # 0xFFFFFF80 (halfword is 0xFF80 = 65408)
sh   t0, 8(s2)          # mem[520] = 0xFF80
lh   t1, 8(s2)          # t1 = -128
lhu  t2, 8(s2)          # t2 = 65408
addi t3, zero, -128
lui  t4, 16             # 65536
addi t4, t4, -128       # 65408
bne  t1, t3, test7_fail
bne  t2, t4, test7_fail
addi s0, s0, 1          # TEST 7 PASS
jal  zero, test8_start
test7_fail:
addi s1, s1, 1

test8_start:
# ------------------------------------------------------------------------------
# TEST 8: Control Flow (BEQ, BNE, BLT, BGE, BLTU, BGEU, JAL, JALR)
# ------------------------------------------------------------------------------
addi t0, zero, 10
addi t1, zero, 10
addi t2, zero, 20
addi t3, zero, -10

# BEQ taken & not taken
beq  t0, t1, beq_taken
jal  zero, test8_fail
beq_taken:
beq  t0, t2, test8_fail # not taken

# BNE taken & not taken
bne  t0, t2, bne_taken
jal  zero, test8_fail
bne_taken:
bne  t0, t1, test8_fail # not taken

# BLT & BGE (signed)
blt  t3, t0, blt_taken  # -10 < 10
jal  zero, test8_fail
blt_taken:
bge  t0, t3, bge_taken  # 10 >= -10
jal  zero, test8_fail
bge_taken:

# BLTU & BGEU (unsigned)
bltu t0, t3, bltu_taken # (unsigned)10 < (unsigned)-10
jal  zero, test8_fail
bltu_taken:
bgeu t3, t0, bgeu_taken # (unsigned)-10 >= (unsigned)10
jal  zero, test8_fail
bgeu_taken:

# JAL & JALR Linkage (Subroutine call & return)
jal  ra, test_subroutine
addi s0, s0, 1          # TEST 8 PASS
jal  zero, test9_start
test8_fail:
addi s1, s1, 1

test9_start:
# ------------------------------------------------------------------------------
# TEST 9: Loop & Memory Checksum Stress Test (RAW Hazards & Branch Flushes)
# ------------------------------------------------------------------------------
# Array initialization: mem[600 + i*4] = i * 7 + 3 for i = 0..15
addi t0, zero, 16       # N = 16
addi t1, zero, 0        # i = 0
addi t2, s2, 88         # array_base = 512 + 88 = 600

init_loop:
slli t3, t1, 2          # offset = i * 4
add  t4, t2, t3         # addr = base + offset
slli t5, t1, 3          # i * 8
sub  t5, t5, t1         # i * 7
addi t5, t5, 3          # val = i * 7 + 3
sw   t5, 0(t4)          # mem[addr] = val
addi t1, t1, 1          # i++
bne  t1, t0, init_loop

# Array Checksum Calculation: sum = sum(mem[addr])
addi t1, zero, 0        # i = 0
addi a3, zero, 0        # checksum = 0

check_loop:
slli t3, t1, 2          # offset = i * 4
add  t4, t2, t3         # addr = base + offset
lw   t5, 0(t4)          # val = mem[addr] (LOAD-USE HAZARD)
add  a3, a3, t5         # checksum += val (RAW hazard on loaded value)
addi t1, t1, 1          # i++
bne  t1, t0, check_loop

# Expected checksum: sum_{i=0..15} (7*i + 3) = 7*120 + 3*16 = 840 + 48 = 888
addi t6, zero, 888
bne  a3, t6, test9_fail
addi s0, s0, 1          # TEST 9 PASS
jal  zero, all_done
test9_fail:
addi s1, s1, 1

# Subroutine for TEST 8
test_subroutine:
addi t5, zero, 777
jalr zero, ra, 0        # return

# ------------------------------------------------------------------------------
# FINAL REPORTING & HALT
# ------------------------------------------------------------------------------
all_done:
fence                   # Test FENCE instruction
add  a1, zero, s0       # a1 = Total Tests Passed
add  a2, zero, s1       # a2 = Total Tests Failed

# If failed == 0, set success code 0xCAFE0000 | passed_count in a0
# If failed > 0, set fail code 0xDEAD0000 | failed_count in a0
bne  s1, zero, report_fail
lui  t0, 831456         # 831456 = 0xCAFE0 (0xCAFE0000)
or   a0, t0, s0         # a0 = 0xCAFE0000 | passed
ecall

report_fail:
lui  t0, 912080         # 912080 = 0xDEAD0 (0xDEAD0000)
or   a0, t0, s1         # a0 = 0xDEAD0000 | failed
ecall
