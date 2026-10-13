# OS Assignment 02: Re-engineering the ls Utility

## 👤 Student Profile
- **Name:** Muhammad Shahid
- **Roll Number:** BSDSF24M054
- **Course:** CS-251 Systems Programming / Operating Systems

## 📋 Project Overview
This project is a custom, fully functional implementation of the Unix `ls` command written completely from scratch in C. It interfaces directly with Linux kernel structures to retrieve, format, and display directory metadata dynamically based on user-supplied argument flags.

## 🚀 Implemented Features
I have incrementally developed and verified all milestone features required by the assignment guidelines:
- **Feature 1:** Baseline environment scaffolding and automated compilation build pipes.
- **Feature 2 (`-l`):** Long listing mode parsing owners, group mappings, byte boundaries, and modification time hooks.
- **Feature 3:** "Down-then-across" column alignment layout querying terminal dimensions dynamically.
- **Feature 4 (`-x`):** Across row-by-row layout engine tracking boundary string widths.
- **Feature 5:** Pure alphabetical sorting pipeline backed by the standard `qsort` library routing string blocks.
- **Feature 6:** Extension sniffing and metadata matching rendering color-coded output formatting natively.
- **Feature 7 (`-R`):** Full deep recursive listings crawling lower directory branches iteratively.

## 🎨 File Type Color Layout
Outputs are styled automatically using standard inline ANSI escape sequences:
- **Blue (Bold):** Sub-directories and folders
- **Green (Bold):** Compiled executables and binary tools
- **Red (Bold):** System archive extensions (`.tar`, `.gz`, `.zip`)
- **Pink (Bold):** Symbolic link maps and descriptors

## 🔧 Compiling and Running
To clean out old intermediate files, compile the binary cleanly, and test the outputs, run this execution trail:

```bash
# Clean workspace and compile the project
make clean && make

# 1. Test basic column listing
./bin/ls

# 2. Test horizontal listing format
./bin/ls -x

# 3. Test detailed metadata overview
./bin/ls -l

# 4. Test cascading deep directory trees
./bin/ls -R

# 5. Test combined features simultaneously
./bin/ls -lR
```
