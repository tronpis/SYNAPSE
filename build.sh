#!/bin/bash
# SYNAPSE SO Build Script
# Licensed under GPLv3

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored message
print_status() {
    echo -e "${BLUE}[*]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[+]${NC} $1"
}

print_error() {
    echo -e "${RED}[-]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

# Check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Check required tools
check_tools() {
    print_status "Checking build tools..."
    
    local missing_tools=()
    
    # Check for GCC with 32-bit support
    if ! command_exists gcc; then
        missing_tools+=("gcc")
    else
        if ! gcc -m32 -v >/dev/null 2>&1; then
            print_warning "GCC found but 32-bit support (gcc-multilib) may be missing"
        fi
    fi
    
    # Check for NASM
    if ! command_exists nasm; then
        missing_tools+=("nasm")
    fi
    
    # Check for LD
    if ! command_exists ld; then
        missing_tools+=("binutils (ld)")
    fi
    
    # Check for GRUB mkrescue
    if ! command_exists grub-mkrescue; then
        missing_tools+=("grub-mkrescue (xorriso)")
    fi
    
    # Check for QEMU (optional, for testing)
    if ! command_exists qemu-system-i386; then
        print_warning "QEMU not found - you won't be able to test the OS"
        print_warning "Install with: apt-get install qemu-system-x86"
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        print_error "Missing required tools: ${missing_tools[*]}"
        echo ""
        echo "To install on Debian/Ubuntu:"
        echo "  sudo apt-get install gcc gcc-multilib nasm binutils grub-pc-bin xorriso qemu-system-x86"
        echo ""
        echo "To install on Fedora:"
        echo "  sudo dnf install gcc gcc-multilib nasm binutils grub2-tools xorriso qemu-system-x86"
        echo ""
        echo "To install on Arch:"
        echo "  sudo pacman -S gcc nasm binutils grub xorriso qemu"
        exit 1
    fi
    
    print_success "All required tools found"
}

# Show build environment info
show_env_info() {
    print_status "Build Environment:"
    echo "  GCC version: $(gcc --version | head -n1)"
    echo "  NASM version: $(nasm --version)"
    echo "  LD version: $(ld --version | head -n1)"
    echo "  GRUB mkrescue: $(grub-mkrescue --version | head -n1)"
    if command_exists qemu-system-i386; then
        echo "  QEMU version: $(qemu-system-i386 --version | head -n1)"
    fi
    echo ""
}

# Clean build artifacts
clean_build() {
    print_status "Cleaning build artifacts..."
    make clean >/dev/null 2>&1 || true
    print_success "Clean complete"
}

# Build the kernel
build_kernel() {
    print_status "Building SYNAPSE SO kernel..."
    
    # Run make
    if make -j$(nproc) all; then
        print_success "Build successful!"
        return 0
    else
        print_error "Build failed!"
        return 1
    fi
}

# Run tests in QEMU
run_tests() {
    print_status "Running tests in QEMU..."
    
    if ! command_exists qemu-system-i386; then
        print_error "QEMU not available for testing"
        return 1
    fi
    
    # Run QEMU with test configuration
    timeout 10 qemu-system-i386 \
        -cdrom synapse.iso \
        -serial stdio \
        -display none \
        -no-reboot \
        -d int,cpu_reset 2>&1 | tee test_output.log || true
    
    # Check test output for success indicators
    if grep -q "\[+\] Kernel initialized successfully" test_output.log; then
        print_success "Kernel boot test passed"
        return 0
    else
        print_error "Kernel boot test failed"
        return 1
    fi
}

# Run the OS in QEMU
run_qemu() {
    print_status "Starting QEMU..."
    
    if ! command_exists qemu-system-i386; then
        print_error "QEMU not installed"
        exit 1
    fi
    
    qemu-system-i386 -cdrom synapse.iso
}

# Run with debugging
debug_qemu() {
    print_status "Starting QEMU with debugging..."
    
    if ! command_exists qemu-system-i386; then
        print_error "QEMU not installed"
        exit 1
    fi
    
    print_status "GDB server will listen on port 1234"
    print_status "In another terminal, run: gdb kernel.elf"
    print_status "Then in GDB: target remote localhost:1234"
    
    qemu-system-i386 -cdrom synapse.iso -s -S
}

# Docker build (optional)
docker_build() {
    print_status "Building in Docker container..."
    
    if ! command_exists docker; then
        print_error "Docker not installed"
        exit 1
    fi
    
    # Create Dockerfile if it doesn't exist
    if [ ! -f Dockerfile ]; then
        print_status "Creating Dockerfile..."
        cat > Dockerfile << 'EOF'
FROM ubuntu:22.04

# Install build dependencies
RUN apt-get update && apt-get install -y \
    gcc \
    gcc-multilib \
    nasm \
    binutils \
    grub-pc-bin \
    xorriso \
    make \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

CMD ["./build.sh"]
EOF
    fi
    
    # Build Docker image
    print_status "Building Docker image..."
    docker build -t synapse-so-builder .
    
    # Run build in container
    print_status "Running build in container..."
    docker run --rm -v "$(pwd):/build" synapse-so-builder ./build.sh build
    
    print_success "Docker build complete"
}

# Show usage
usage() {
    echo "SYNAPSE SO Build Script"
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  check      - Check if all required tools are installed"
    echo "  build      - Build the kernel (default)"
    echo "  rebuild    - Clean and build"
    echo "  clean      - Remove build artifacts"
    echo "  run        - Build and run in QEMU"
    echo "  debug      - Build and run in QEMU with GDB server"
    echo "  test       - Build and run automated tests"
    echo "  docker     - Build using Docker (clean environment)"
    echo "  help       - Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0              # Build the kernel"
    echo "  $0 rebuild      # Clean and rebuild"
    echo "  $0 run          # Build and run in QEMU"
    echo "  $0 docker       # Build in Docker container"
    echo ""
}

# Main script
main() {
    local cmd="${1:-build}"
    
    case "$cmd" in
        check)
            check_tools
            show_env_info
            ;;
        build)
            check_tools
            build_kernel
            ;;
        rebuild)
            check_tools
            clean_build
            build_kernel
            ;;
        clean)
            clean_build
            ;;
        run)
            check_tools
            build_kernel && run_qemu
            ;;
        debug)
            check_tools
            build_kernel && debug_qemu
            ;;
        test)
            check_tools
            build_kernel && run_tests
            ;;
        docker)
            docker_build
            ;;
        help|--help|-h)
            usage
            ;;
        *)
            print_error "Unknown command: $cmd"
            echo ""
            usage
            exit 1
            ;;
    esac
}

# Run main function
main "$@"
