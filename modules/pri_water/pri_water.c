#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_TRACK 16

typedef struct {
    char name[256];
    time_t last_mtime;
    int stable_count;
} track_t;

static track_t track[MAX_TRACK];
static int track_n = 0;

track_t* find_track(const char *name) {
    for (int i = 0; i < track_n; i++)
        if (strcmp(track[i].name, name) == 0) return &track[i];
    if (track_n < MAX_TRACK) {
        strncpy(track[track_n].name, name, sizeof(track[track_n].name) - 1);
        track[track_n].last_mtime = 0;
        track[track_n].stable_count = 0;
        return &track[track_n++];
    }
    return NULL;
}

int main() {
    const char *dir = "/data/vendor/.water";

    while (1) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *ent;
            char path[512];
            while ((ent = readdir(d)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);

                struct stat st;
                if (stat(path, &st) != 0) continue;

                chmod(path, 0664);

                track_t *t = find_track(ent->d_name);
                if (t) {
                    if (st.st_mtime == t->last_mtime) {
                        t->stable_count++;
                        if (t->stable_count >= 2) {
                            unlink(path);
                            t->stable_count = 0;
                            t->last_mtime = 0;
                        }
                    } else {
                        t->last_mtime = st.st_mtime;
                        t->stable_count = 0;
                    }
                }
            }
            closedir(d);
        }
        sleep(1);
    }
    return 0;
}
