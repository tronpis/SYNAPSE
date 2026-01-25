#!/bin/bash
# Fix Makefile line 112 to use tabs
python3 << 'EOF'
with open("Makefile", "r") as f:
    lines = f.readlines()
# Fix line 111 (index 110)
lines[111] = "\t@mkdir -p $(BUILD_DIR)\n"
with open("Makefile", "w") as f:
    f.writelines(lines)
print("Fixed Makefile")
EOF
