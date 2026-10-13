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

// Modes representation enum for strict task assignment definitions
enum { MODE_DEFAULT, MODE_LONG, MODE_HORIZ };

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

    // 1. Parse both flags (-l and -x) inside getopt loop
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

    // Dynamic storage matrix structures
    char **names = NULL;
    int n = 0;
    int capacity = 0;
    int maxlen = 0;

    struct dirent *ep;
    while ((ep = readdir(dir)) != NULL) {
        if (ep->d_name[0] == '.') {
            continue;
        }

        // Collect names for default columns and horizontal layout sorting
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

    // Get terminal screen width configuration structures
    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        width = w.ws_col;
    }
    int colw = maxlen + 2;

    // 2. Output handling block depending on requested mode execution states
    if (mode == MODE_LONG) {
        for (int i = 0; i < n; i++) {
            char full_path[4096];
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

            printf("%s %3lu %-8s %-8s %8ld %.12s %s\n", 
                   modestr, (unsigned long)st.st_nlink, 
                   pw ? pw->pw_name : "?", gr ? gr->gr_name : "?", 
                   (long)st.st_size, t + 4, names[i]);
        }
    } 
    else if (mode == MODE_HORIZ && n > 0) {
        // Feature 4: Across left-to-right printing matrix pattern
        int pos = 0;
        for (int i = 0; i < n; i++) {
            if (pos + colw > width) {
                printf("\n");
                pos = 0;
            }
            printf("%-*s", colw, names[i]);
            pos += colw;
        }
        printf("\n");
    } 
    else if (mode == MODE_DEFAULT && n > 0) {
        // Feature 3: Down-then-across layout processing state flow
        int cols = width / colw; 
        if (cols < 1) cols = 1;
        int rows = (n + cols - 1) / cols;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int idx = r + c * rows;
                if (idx < n) {
                    printf("%-*s", colw, names[idx]);
                }
            }
            printf("\n");
        }
    }

    // Clean allocated runtime memory explicit steps execution target
    for (int i = 0; i < n; i++) {
        free(names[i]);
    }
    free(names);

    return 0;
}
