#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Usage: $0 <simulator> [program] [args...]"
    exit 1
fi

# ==============================
# 自动拆分：模拟器 + 所有后续参数
# ==============================
SIMULATOR="$1"
shift
PROGRAM_ARGS="$@"

FULL_CMD="$SIMULATOR $PROGRAM_ARGS"
REPORT="performance_report.txt"

echo "============================================="
echo "        Simulator Performance Analyzer"
echo "============================================="
echo "Simulator: $SIMULATOR"
echo "Command:   $SIMULATOR $PROGRAM_ARGS"
echo "Date:      $(date)"
echo "============================================="
echo ""

# ==============================
# 1. Basic Timing
# ==============================
echo "[1/5] Basic Execution Time"
TIME_OUTPUT=$( { time $FULL_CMD > /dev/null 2>&1; } 2>&1 )
REAL=$(echo "$TIME_OUTPUT" | awk '/real/ {print $2}')
USER=$(echo "$TIME_OUTPUT" | awk '/user/ {print $2}')
SYS=$(echo "$TIME_OUTPUT" | awk '/sys/ {print $2}')

echo "  Real time:  ${REAL}s"
echo "  User time:  ${USER}s"
echo "  System time:${SYS}s"
echo ""

# ==============================
# 2. Hardware Performance Counters
# ==============================
echo "[2/5] Hardware Events (perf stat)"
perf stat -e cycles,instructions,branches,branch-misses,L1i-cache-misses,L1d-cache-misses \
    -o perf_temp.log $FULL_CMD > /dev/null 2>&1

cat perf_temp.log | grep -E \
"cycles|instructions|branches|branch-misses|L1i-cache-misses|L1d-cache-misses" \
| sed 's/^/  /'

echo ""

# ==============================
# 3. Top Hot Functions
# ==============================
echo "[3/5] Top CPU Hot Functions"
perf record -g -o perf_temp.data $FULL_CMD > /dev/null 2>&1
FUNCTIONS=$(perf report -i perf_temp.data --stdio -n 2>/dev/null \
    | grep -A15 "Overhead" \
    | grep -E '^[ ]+[0-9]+\.[0-9]+%' \
    | head -6)

if [ -z "$FUNCTIONS" ]; then
    echo "  (No function data available. Run with sudo for better results.)"
else
    echo "$FUNCTIONS" | sed 's/^/  /'
fi
echo ""

# ==============================
# 4. Automatic Bottleneck Analysis
# ==============================
echo "[4/5] Bottleneck Analysis"

L1I_MISS=$(grep L1i-cache-misses perf_temp.log | awk '{print $1}' | tr -d ',')
BR_MISS=$(grep branch-misses perf_temp.log | awk '{print $1}' | tr -d ',')

if [ -n "$L1I_MISS" ] && [ $L1I_MISS -gt 500000 ] 2>/dev/null; then
    echo "  ⚠️  HIGH L1i-Cache Miss → I-Cache bottleneck (code too large)"
else
    echo "  ✅ L1i-Cache Miss rate is normal"
fi

if [ -n "$BR_MISS" ] && [ $BR_MISS -gt 500000 ] 2>/dev/null; then
    echo "  ⚠️  HIGH Branch Misses → Dispatch/jump overhead is high"
else
    echo "  ✅ Branch prediction is normal"
fi
echo ""

# ==============================
# 5. Full Report Export
# ==============================
echo "[5/5] Generating full report: $REPORT"

cat > "$REPORT" <<EOF
=====================================================================
               RISC-V Simulator Performance Report
=====================================================================
Simulator:  $SIMULATOR
Arguments: $PROGRAM_ARGS
Full cmd:  $SIMULATOR $PROGRAM_ARGS
Date:      $(date)

Timing:
  Real: ${REAL}s
  User: ${USER}s
  Sys:  ${SYS}s

Raw perf events:
$(cat perf_temp.log | grep -v "Performance counter stats")

Bottleneck Summary:
$( [ -n "$L1I_MISS" ] && [ $L1I_MISS -gt 500000 ] && echo "  - L1i-Cache Miss too high" )
$( [ -n "$BR_MISS" ] && [ $BR_MISS -gt 500000 ] && echo "  - Branch Misses too high" )

Top Functions:
$FUNCTIONS
=====================================================================
EOF

# Cleanup
rm -f perf_temp.log perf_temp.data > /dev/null 2>&1

echo "Done! Report saved to: $REPORT"
echo "============================================="
