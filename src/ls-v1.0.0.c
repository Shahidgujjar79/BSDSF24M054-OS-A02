#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>

enum { MODE_DEFAULT, MODE_LONG, MODE_HORIZ };

// ANSI Color Escape Codes Configuration Structures
#define COLOR_RESET   "\x1b[0m"
#define COLOR_BLUE    "\x1b[1;34m"
#define COLOR_GREEN   "\x1b[1;32m"
#define COLOR_RED     "\x1b[1;31m"
#define COLOR_PINK    "\x1b[1;35m"

int cmp_names(const void *a, const void *b) {
    const char *x = *(const char **)a;
    const char *y = *(const char **)b;
    return strcmp(x, y);
}

// Helper function to check if file has archive extensions
int is_archive(const char *name) {
    size_t len = strlen(name);
    if (len > 4 && strcmp(name + len - 4, ".zip") == 0) return 1;
    if (len > 4 && strcmp(name + len - 4, ".tar") == 0) return 1;
    if (len > 3 && strcmp(name + len - 3, ".gz") == 0) return 1;
    return 0;
}

// Feature 6: Dynamic color resolution handler based on metadata
void print_colorized(const char *name, mode_t mode, int pad_len) {
    const char *color = COLOR_RESET;

    if (S_ISDIR(mode)) {
        color = COLOR_BLUE;
    } else if (S_ISLNK(mode)) {
        color = COLOR_PINK;
    } else if (is_archive(name)) {
        color = COLOR_RED;
    } else if (mode & S_IXUSR) {
        color = COLOR_GREEN;
    }

    if (pad_len > 0) {
        // Handle grid aligned padding keeping ANSI codes out of spacing calculations
        char fmt[32];
        snprintf(fmt, sizeof(fmt), "%s%%-%ds%s", color, pad_len, COLOR_RESET);
        printf(fmt, name);
    } else {
        printf("%s%s%s", color, name, COLOR_RESET);
    }
}

void mode_to_str(mode_t m, char s[]) {
    s[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' :
           S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    s[1] = (m & S_IRUSR) ? 'r' : '-'; s[2] = (m & S_IWUSR) ? 'w' : '-';
    s[3] = (m & S_IXUSR) ? 'x' : '-';
    s[4] = (m & S_IRGRP) ? 'r' : '-'; s[5] = (m & S_IWGRP) ? 'w' : '-';
    s[6] = (m & S_IXGRP) ? 'x' : '-';
    s[7] = (m & S_IROTH) ? 'r' : '-'; s[8] = (m & S_IWOTH) ? 'w' : '-';
    s[9] = (m & S_IXOTH) ? 'x' : '-';
    
    if (m & S_ISUID) s[3] = (s[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) s[6] = (s[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) s[9] = (s[9] == 'x') ? 't' : 'T';
    s[10] = '\0';
}

int main(int argc, char *argv[]) {
    int opt;
    int mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                mode = MODE_LONG;
                break;
            case 'x':
                mode = MODE_HORIZ;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-x] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *dir_path = ".";
    if (optind < argc) {
        dir_path = argv[optind];
    }

    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Cannot open directory");
        exit(EXIT_FAILURE);
    }

    char **names = NULL;
    int n = 0;
    int capacity = 0;
    int maxlen = 0;

    struct dirent *ep;
    while ((ep = readdir(dir)) != NULL) {
        if (ep->d_name[0] == '.') {
            continue;
        }

        if (n >= capacity) {
            capacity = capacity == 0 ? 16 : capacity * 2;
            names = realloc(names, capacity * sizeof(char *));
        }
        names[n] = strdup(ep->d_name);
        int len = strlen(names[n]);
        if (len > maxlen) {
            maxlen = len;
        }
        n++;
    }
    closedir(dir);

    if (n > 0) {
        qsort(names, n, sizeof(char *), cmp_names);
    }

    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        width = w.ws_col;
    }
    int colw = maxlen + 2;

    if (mode == MODE_LONG) {
        for (int i = 0; i < n; i++) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, names[i]);

            struct stat st;
            if (lstat(full_path, &st) == -1) {
                continue;
            }

            char modestr[11];
            mode_to_str(st.st_mode, modestr);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);
            char *t = ctime(&st.st_mtime);

            printf("%s %3lu %-8s %-8s %8ld %.12s ", 
                   modestr, (unsigned long)st.st_nlink, 
                   pw ? pw->pw_name : "?", gr ? gr->gr_name : "?", 
                   (long)st.st_size, t + 4);
            
            // Feature 6 long listing target parameter extension color rendering
            print_colorized(names[i], st.st_mode, 0);
            printf("\n");
        }
    } 
    else if (mode == MODE_HORIZ && n > 0) {
        int pos = 0;
        for (int i = 0; i < n; i++) {
            char full_path[512];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, names[i]);
            struct stat st;
            mode_t m = 0;
            if (lstat(full_path, &st) == 0) m = st.st_mode;

            if (pos + colw > width) {
                printf("\n");
                pos = 0;
            }
            print_colorized(names[i], m, colw);
            pos += colw;
        }
        printf("\n");
    } 
    else if (mode == MODE_DEFAULT && n > 0) {
        int cols = width / colw; 
        if (cols < 1) cols = 1;
        int rows = (n + cols - 1) / cols;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int idx = r + c * rows;
                if (idx < n) {
                    char full_path[512];
                    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, names[idx]);
                    struct stat st;
                    mode_t m = 0;
                    if (lstat(full_path, &st) == 0) m = st.st_mode;

                    print_colorized(names[idx], m, colw);
                }
            }
            printf("\n");
        }
    }

    for (int i = 0; i < n; i++) {
        free(names[i]);
    }
    free(names);

    return 0;
}

