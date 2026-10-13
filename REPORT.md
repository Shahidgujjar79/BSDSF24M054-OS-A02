# OS Assignment 02: Re-engineering the ls Utility - Report

## 👤 Student Information
- **Name:** Muhammad Shahid
- **Roll Number:** BSDSF24M054
- **Repository:** BSDSF24M054-OS-A02

---

## 🛠️ Feature Implementation & System Architecture

### Feature 1: Project Setup (v1.0.0)
- **Staging Structure:** Created the base directories (`src`, `bin`, `obj`, `man`) to keep things organized.
- **Compiler Rules:** Wrote a custom `Makefile` from scratch. Added flags like `-Wall -Wextra -g` so the compiler catches errors early and allows debugging if the pointers act up.

### Feature 2: Long Listing Format (v1.1.0)
- **System Calls:** `lstat()`, `getpwuid()`, `getgrgid()`, `ctime()`.
- **Implementation Strategy:** 
  - Used `lstat` instead of regular `stat` because we don't want symlinks to break the file checks.
  - Wrote a helper function `mode_to_str` that uses bitwise AND (`&`) with permissions macros (like `S_IRUSR`, `S_IWUSR`) to map out strings like `drwxrwxr-x`.
  - Gathered UID and GID data, fed them into `getpwuid` and `getgrgid` to fetch actual names instead of ID numbers, and cleaned up the `ctime` string formatting to print smoothly.

### Feature 3: Column Display (v1.2.0)
- **System Calls:** `ioctl()` with `TIOCGWINSZ`.
- **Implementation Strategy:** 
  - To stop file names from printing on a single giant line or breaking awkwardly, I used `ioctl` to check the actual current column width of the terminal.
  - Saved filenames into a dynamic string array (`char** names`) using `realloc` and tracked down the longest name (`maxlen`) to manage column gaps.
  - Implemented the down-then-across loop using the formula `idx = r + c * rows` so the folder items print top-to-bottom first, then left-to-right.

### Feature 4: Horizontal Display (v1.3.0)
- **Flags Handled:** Updated the `getopt()` loop to catch the `-x` flag option alongside `-l`.
- **Implementation Strategy:** Instead of printing downwards, I modified the loop to track the cursor's character position across the row (`pos`). If adding the next item exceeds the terminal width boundary (`pos + colw > width`), it prints a newline, resets the counter, and prints on the next line.

### Feature 5: Alphabetical Sort (v1.4.0)
- **Algorithms Used:** C Standard Library `qsort()`.
- **Implementation Strategy:** Since `ls` outputs need to be ordered, I threw all the array names into `qsort`. Wrote a basic comparator function (`cmp_names`) that casts the pointers down to `const char**`, un-references them, and passes them to standard `strcmp` for dictionary sorting.

### Feature 6: Colorized Output (v1.5.0)
- **Implementation Strategy:** Used standard ANSI color codes (`\x1b[1;34m` for folders, etc.). Added basic string logic inside a wrapper to check extensions like `.zip`, `.tar`, and `.gz`. The main challenge was passing padding rules through the custom format string so the color codes do not break the column widths.

### Feature 7: Recursive Listing (v1.6.0)
- **System Calls:** `opendir()`, `readdir()`, `closedir()`.
- **Implementation Strategy:** Cleaned up `main()` by pulling the whole directory loop out into a dedicated function called `list_directory`. If the `-R` flag is passed, the function loops through the sorted names after printing. If it encounters a valid folder using `S_ISDIR`, it appends the new sub-path using `snprintf` and calls itself recursively.

---

## 🔬 System Call Reference Analysis

1. **`stat()` vs `lstat()`:** Regular `stat()` follows a symlink all the way to its source file to extract metadata, whereas `lstat()` stops right at the link itself and returns the link container's actual details.
2. **`getpwuid()` & `getgrgid()`:** These system calls take the raw numeric owner ID and group ID numbers stored in the file system and look up the machine's internal users database to convert them into readable string text.
3. **`ioctl()` Window Geometry Tracking:** This call is used to talk directly to the terminal's hardware drivers. Passing `TIOCGWINSZ` returns a struct containing the precise row and column width of the open terminal window.
