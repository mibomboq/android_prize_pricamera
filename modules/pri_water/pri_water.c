#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

#define MAX_TRACK 64
#define STABLE_NEEDED 5

typedef struct {
    char name[256];
    time_t last_mtime;
    int stable_count;
    int wrote;
    int used;
} track_t;

static track_t track[MAX_TRACK];

static track_t *find_track(const char *name)
{
    for (int i = 0; i < MAX_TRACK; i++)
        if (track[i].used && strcmp(track[i].name, name) == 0)
            return &track[i];
    for (int i = 0; i < MAX_TRACK; i++) {
        if (!track[i].used) {
            memset(&track[i], 0, sizeof(track[i]));
            strncpy(track[i].name, name, sizeof(track[i].name) - 1);
            track[i].used = 1;
            return &track[i];
        }
    }
    return NULL;
}

static void drop_track(track_t *t)
{
    memset(t, 0, sizeof(*t));
}

static int file_is_open(const char *path)
{
    DIR *proc = opendir("/proc");
    if (!proc)
        return 0;
    struct dirent *pe;
    while ((pe = readdir(proc)) != NULL) {
        if (!isdigit((unsigned char)pe->d_name[0]))
            continue;
        char fd_dir[PATH_MAX];
        snprintf(fd_dir, sizeof(fd_dir), "/proc/%s/fd", pe->d_name);
        DIR *fd = opendir(fd_dir);
        if (!fd)
            continue;
        struct dirent *fe;
        while ((fe = readdir(fd)) != NULL) {
            if (fe->d_name[0] == '.')
                continue;
            char link[PATH_MAX], target[PATH_MAX];
            snprintf(link, sizeof(link), "%s/%s", fd_dir, fe->d_name);
            ssize_t n = readlink(link, target, sizeof(target) - 1);
            if (n > 0) {
                target[n] = '\0';
                if (strcmp(target, path) == 0) {
                    closedir(fd);
                    closedir(proc);
                    return 1;
                }
            }
        }
        closedir(fd);
    }
    closedir(proc);
    return 0;
}

int main(void)
{
    const char *dir = "/data/vendor/.water";

    while (1) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *ent;
            char path[512];
            while ((ent = readdir(d)) != NULL) {
                if (strcmp(ent->d_name, ".") == 0 ||
                    strcmp(ent->d_name, "..") == 0)
                    continue;
                snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
                struct stat st;
                if (stat(path, &st) != 0)
                    continue;
                if (S_ISDIR(st.st_mode))
                    continue;

                track_t *t = find_track(ent->d_name);
                if (!t)
                    continue;

                if (!t->wrote) {
                    if (t->last_mtime == 0) {
                        t->last_mtime = st.st_mtime;
                    } else if (st.st_mtime != t->last_mtime) {
                        t->wrote = 1;
                        t->last_mtime = st.st_mtime;
                        t->stable_count = 0;
                    }
                } else {
                    if (st.st_mtime != t->last_mtime) {
                        t->last_mtime = st.st_mtime;
                        t->stable_count = 0;
                    } else if (++t->stable_count >= STABLE_NEEDED) {
                        if (!file_is_open(path)) {
                            unlink(path);
                            drop_track(t);
                        } else {
                            t->stable_count = 0;
                        }
                    }
                }
            }
            closedir(d);
        }
        sleep(1);
    }
    return 0;
}
