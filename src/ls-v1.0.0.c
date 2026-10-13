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

// Function prototype for converting st_mode to an ls-style permission string
void mode_to_str(mode_t m, char s[11]) {
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
    
    // Handle special permission flags (SUID, SGID, Sticky Bit)
    if (m & S_ISUID) s[3] = (s[3] == 'x') ? 's' : 'S';
    if (m & S_ISGID) s[6] = (s[6] == 'x') ? 's' : 'S';
    if (m & S_ISVTX) s[9] = (s[9] == 'x') ? 't' : 'T';
    s[10] = '\0';
}

int main(int argc, char *argv[]) {
    int opt;
    int long_flag = 0;

    // 1. Command-line parsing with getopt
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

    // 2. Determine target listing directory (defaults to current directory)
    const char *dir_path = ".";
    if (optind < argc) {
        dir_path = argv[optind];
    }

    // 3. Open target directory stream
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Cannot open directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *ep;
    // 4. Iterate over items in the directory
    while ((ep = readdir(dir)) != NULL) {
        // Skip hidden dot-files by default to match standard ls behavior
        if (ep->d_name[0] == '.') {
            continue;
        }

        // Build absolute path or relative path to get accurate file statistics
        char full_path[4096];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, ep->d_name);

        struct stat st;
        if (lstat(full_path, &st) == -1) {
            perror("lstat failed");
            continue;
        }

        // 5. Output rendering depending on flags
        if (long_flag) {
            char modestr[11];
            mode_to_str(st.st_mode, modestr);

            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);
            char *t = ctime(&st.st_mtime);

            // Format layout matching teacher rules precisely
            printf("%s %3lu %-8s %-8s %8ld %.12s %s\n", 
                   modestr,
                   (unsigned long)st.st_nlink, 
                   pw ? pw->pw_name : "?",
                   gr ? gr->gr_name : "?", 
                   (long)st.st_size, 
                   t + 4, 
                   ep->d_name);
        } else {
            // Default standard brief format output
            printf("%s\n", ep->d_name);
        }
    }

    closedir(dir);
    return 0;
}

