#!/bin/bash
# SYNAPSE SO - Phase 3 Automated Test Script
# Licensed under GPLv3

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

print_header() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}$1${NC}"
    echo -e "${BLUE}========================================${NC}"
}

print_test() {
    echo -e "${YELLOW}[TEST]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    print_test "$test_name"
    
    if eval "$test_cmd" | grep -q "$expected"; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        print_pass "$test_name"
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        print_fail "$test_name"
        return 1
    fi
}

# Main test suite
print_header "SYNAPSE SO - Phase 3 Test Suite"

print_info "Testing Phase 3: User Mode & System Calls"
echo ""

# Test 1: Build test
print_test "Build Test - Kernel compiles successfully"
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "Kernel built successfully"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "Kernel build failed"
    exit 1
fi
TESTS_RUN=$((TESTS_RUN + 1))

echo ""

# Test 2: File existence test
print_test "File Existence - Required files present"
FILES=(
    "kernel/usermode.c"
    "kernel/include/kernel/usermode.h"
    "docs/PHASE3_USER_MODE.md"
    "docs/PHASE3_STATUS.md"
    "PHASE3_COMPLETION.md"
)

all_files_exist=true
for file in "${FILES[@]}"; do
    if [ -f "$file" ]; then
        print_pass "Found $file"
    else
        print_fail "Missing $file"
        all_files_exist=false
    fi
done

TESTS_RUN=$((TESTS_RUN + 1))
if [ "$all_files_exist" = true ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
fi

echo ""

# Test 3: Code quality checks
print_test "Code Quality - Check for basic issues"

# Check for common issues
issues_found=false

# Check for TODO/FIXME
if grep -r "TODO\|FIXME" kernel/*.c kernel/include/kernel/*.h > /dev/null 2>&1; then
    print_info "Found TODO/FIXME markers (acceptable)"
fi

# Check for magic numbers (basic check)
if grep -E "0x[0-9A-Fa-f]{8}" kernel/usermode.c | grep -v "0x400000\|0x800000\|0xC0000000\|PAGE_" > /dev/null 2>&1; then
    print_info "Found some magic numbers (may need review)"
fi

# Check for proper includes
if grep -q "#include <kernel/usermode.h>" kernel/kernel.c; then
    print_pass "kernel.c includes usermode.h"
else
    print_fail "kernel.c missing usermode.h include"
    issues_found=true
fi

TESTS_RUN=$((TESTS_RUN + 1))
if [ "$issues_found" = false ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "Code quality checks passed"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "Code quality checks found issues"
fi

echo ""

# Test 4: Documentation check
print_test "Documentation - Verify completeness"

doc_complete=true

# Check PHASE3_USER_MODE.md has key sections
key_sections=(
    "User Mode Transition"
    "Security Model"
    "Testing"
    "Performance"
)

for section in "${key_sections[@]}"; do
    if grep -q "$section" docs/PHASE3_USER_MODE.md; then
        print_pass "Found section: $section"
    else
        print_fail "Missing section: $section"
        doc_complete=false
    fi
done

TESTS_RUN=$((TESTS_RUN + 1))
if [ "$doc_complete" = true ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "Documentation complete"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "Documentation incomplete"
fi

echo ""

# Test 5: API check
print_test "API Check - Verify function signatures"

api_correct=true

# Check for enter_usermode
if grep -q "void enter_usermode(uint32_t entry_point, uint32_t user_stack)" kernel/include/kernel/usermode.h; then
    print_pass "enter_usermode signature correct"
else
    print_fail "enter_usermode signature missing/incorrect"
    api_correct=false
fi

# Check for create_user_test_process
if grep -q "uint32_t create_user_test_process(void)" kernel/include/kernel/usermode.h; then
    print_pass "create_user_test_process signature correct"
else
    print_fail "create_user_test_process signature missing/incorrect"
    api_correct=false
fi

TESTS_RUN=$((TESTS_RUN + 1))
if [ "$api_correct" = true ]; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "API signatures correct"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "API signatures incorrect"
fi

echo ""

# Test 6: Makefile check
print_test "Build System - Verify Makefile updated"

if grep -q "usermode.c" Makefile; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "Makefile includes usermode.c"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "Makefile missing usermode.c"
fi
TESTS_RUN=$((TESTS_RUN + 1))

echo ""

# Test 7: README check
print_test "README - Verify Phase 3 status"

if grep -q "Fase 3.*COMPLETADA" README.md; then
    TESTS_PASSED=$((TESTS_PASSED + 1))
    print_pass "README shows Phase 3 complete"
else
    TESTS_FAILED=$((TESTS_FAILED + 1))
    print_fail "README not updated for Phase 3"
fi
TESTS_RUN=$((TESTS_RUN + 1))

echo ""

# Summary
print_header "Test Summary"
echo ""
echo -e "Total Tests:  ${TESTS_RUN}"
echo -e "${GREEN}Passed:       ${TESTS_PASSED}${NC}"
echo -e "${RED}Failed:       ${TESTS_FAILED}${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  ALL TESTS PASSED! ✓${NC}"
    echo -e "${GREEN}========================================${NC}"
    exit 0
else
    echo -e "${RED}========================================${NC}"
    echo -e "${RED}  SOME TESTS FAILED! ✗${NC}"
    echo -e "${RED}========================================${NC}"
    exit 1
fi
