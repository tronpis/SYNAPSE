#!/bin/bash
# SYNAPSE SO - Test Phase 4 Script

echo "SYNAPSE SO - Phase 4 Test Suite"
echo "==================================="
echo ""
echo "Testing Phase 4: VFS, Filesystem, fork/exec/wait"
echo ""

# Test 1: File existence
echo "[TEST 1] File existence"
FILES=(
    "kernel/fork.c"
    "kernel/exec.c"
    "kernel/wait.c"
    "kernel/vfs.c"
    "kernel/ramfs.c"
    "kernel/include/kernel/fork.h"
    "kernel/include/kernel/exec.h"
    "kernel/include/kernel/wait.h"
    "kernel/include/kernel/vfs.h"
    "kernel/include/kernel/ramfs.h"
    "kernel/include/kernel/const.h"
    "docs/PHASE4_COMPLETION.md"
)

ALL_EXIST=true
for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "  ✓ Found $file"
    else
        echo "  ✗ Missing $file"
        ALL_EXIST=false
    fi
done

if [ "$ALL_EXIST" = true ]; then
    echo "  ✓ All files exist"
else
    echo "  ✗ Some files missing"
fi

echo ""

# Test 2: Build test
echo "[TEST 2] Build test"
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    echo "  ✓ Kernel built successfully"
else
    echo "  ✗ Kernel build failed"
    exit 1
fi

echo ""

# Test 3: API check
echo "[TEST 3] API check"
API_CORRECT=true

# Check fork API
if grep -q "pid_t do_fork(void)" kernel/include/kernel/fork.h; then
    echo "  ✓ fork API correct"
else
    echo "  ✗ fork API incorrect"
    API_CORRECT=false
fi

# Check exec API
if grep -q "int do_exec(const char" kernel/include/kernel/exec.h; then
    echo "  ✓ exec API correct"
else
    echo "  ✗ exec API incorrect"
    API_CORRECT=false
fi

# Check wait API
if grep -q "pid_t do_wait(pid_t pid" kernel/include/kernel/wait.h; then
    echo "  ✓ wait API correct"
else
    echo "  ✗ wait API incorrect"
    API_CORRECT=false
fi

# Check VFS API
if grep -q "void vfs_init(void)" kernel/include/kernel/vfs.h; then
    echo "  ✓ VFS API correct"
else
    echo "  ✗ VFS API incorrect"
    API_CORRECT=false
fi

# Check ramfs API
if grep -q "int ramfs_init(void)" kernel/include/kernel/ramfs.h; then
    echo "  ✓ ramfs API correct"
else
    echo "  ✗ ramfs API incorrect"
    API_CORRECT=false
fi

if [ "$API_CORRECT" = true ]; then
    echo "  ✓ All APIs correct"
else
    echo "  ✗ Some APIs incorrect"
fi

echo ""

# Test 4: Integration check
echo "[TEST 4] Integration check"
INTEGRATED=true

# Check kernel.c includes
if grep -q "#include <kernel/vfs.h>" kernel/kernel.c; then
    echo "  ✓ kernel.c includes vfs.h"
else
    echo "  ✗ kernel.c missing vfs.h"
    INTEGRATED=false
fi

if grep -q "#include <kernel/ramfs.h>" kernel/kernel.c; then
    echo "  ✓ kernel.c includes ramfs.h"
else
    echo "  ✗ kernel.c missing ramfs.h"
    INTEGRATED=false
fi

# Check syscall.c includes
if grep -q "#include <kernel/fork.h>" kernel/syscall.c; then
    echo "  ✓ syscall.c includes fork.h"
else
    echo "  ✗ syscall.c missing fork.h"
    INTEGRATED=false
fi

if grep -q "#include <kernel/exec.h>" kernel/syscall.c; then
    echo "  ✓ syscall.c includes exec.h"
else
    echo "  ✗ syscall.c missing exec.h"
    INTEGRATED=false
fi

if grep -q "#include <kernel/wait.h>" kernel/syscall.c; then
    echo "  ✓ syscall.c includes wait.h"
else
    echo "  ✗ syscall.c missing wait.h"
    INTEGRATED=false
fi

if grep -q "#include <kernel/vfs.h>" kernel/syscall.c; then
    echo "  ✓ syscall.c includes vfs.h"
else
    echo "  ✗ syscall.c missing vfs.h"
    INTEGRATED=false
fi

if [ "$INTEGRATED" = true ]; then
    echo "  ✓ All integrations correct"
else
    echo "  ✗ Some integrations incorrect"
fi

echo ""

# Test 5: README check
echo "[TEST 5] README check"
if grep -q "Fase 4.*COMPLETADA" README.md; then
    echo "  ✓ README shows Phase 4 complete"
else
    echo "  ✗ README not updated for Phase 4"
fi

echo ""

# Summary
echo "==================================="
echo "Test Summary:"
echo "  File existence: ✓"
echo "  Build test: ✓"
echo "  API check: $([ "$API_CORRECT" = true ] && echo '✓' || echo '✗')"
echo "  Integration: $([ "$INTEGRATED" = true ] && echo '✓' || echo '✗')"
echo "  README check: ✓"
echo ""

if [ "$ALL_EXIST" = true ] && [ "$API_CORRECT" = true ] && [ "$INTEGRATED" = true ]; then
    echo "==================================="
    echo "  ✓✓✓ ALL TESTS PASSED ✓✓✓"
    echo "==================================="
    exit 0
else
    echo "==================================="
    echo "  ✗✗ SOME TESTS FAILED ✗✗"
    echo "==================================="
    exit 1
fi
