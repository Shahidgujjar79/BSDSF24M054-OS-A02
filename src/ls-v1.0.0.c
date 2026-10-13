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

void mode_to_str(mode_t m, char s[]) {
    s[0] = S_ISDIR(m) ? 'd' : S_ISLNK(m) ? 'l' : S_ISCHR(m) ? 'c' :
           S_ISBLK(m) ? 'b' : S_ISFIFO(m) ? 'p' : S_ISSOCK(m) ? 's' : '-';
    s[1] = (m & S_IRUSR) ? 'r' : '-'; 
    s[2] = (m & S_IWUSR) ? 'w' : '-';
    s[3] = (m & S_IXUSR) ? 'x' : '-';
    s[4] = (m & S_IRGRP) ? 'r' : '-'; 
    s[5] = (m & S_IWGRP) ? 'w' : '-';
    s[6] = (m & S_IXGRP) ? 'x' : '-';
    s[7] = (m & S_IROTH) ? 'r' : '-'; 
    s[8] = (m & S_IWOTH) ? 'w' : '-';
    s[9] = (m & S_IXOTH) ? 'x' : '-';
    
    if (m & S_ISUID) s[3] = (s[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) s[6] = (s[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) s[9] = (s[9] == 'x') ? 't' : 'T';
    s[10] = '\0';
}

int main(int argc, char *argv[]) {
    int opt;
    int long_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [directory]\n", argv[0]);
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

    // Dynamic array storage structures for Column Display
    char **names = NULL;
    int n = 0;
    int capacity = 0;
    int maxlen = 0;

    struct dirent *ep;
    while ((ep = readdir(dir)) != NULL) {
        if (ep->d_name[0] == '.') {
            continue;
        }

        // Long listing needs absolute checks inline
        if (long_flag) {
            char full_path[4096];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ep->d_name);

            struct stat st;
            if (lstat(full_path, &st) == -1) {
                perror("lstat failed");
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
                   (long)st.st_size, t + 4, ep->d_name);
        } else {
            // Collect names inside the dynamic array allocation for column printing
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
    }
    closedir(dir);

    // 5. Default Brief Column display grid printer execution layout
    if (!long_flag && n > 0) {
        struct winsize w;
        int width = 80; // fallback standard layout width
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
            width = w.ws_col;
        }

        int colw = maxlen + 2; // Longest item name + 2 spacing buffers
        int cols = width / colw; 
        if (cols < 1) cols = 1;
        int rows = (n + cols - 1) / cols;

        // Down-then-across structural nested printing loop
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                int idx = r + c * rows;
                if (idx < n) {
                    printf("%-*s", colw, names[idx]);
                }
            }
            printf("\n");
        }

        // Clean allocated runtime memory blocks explicitly
        for (int i = 0; i < n; i++) {
            free(names[i]);
        }
        free(names);
    }

    return 0;
}

