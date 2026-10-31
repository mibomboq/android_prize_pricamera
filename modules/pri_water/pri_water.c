#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>

/*
 * pri_water v7 — v6 DIPERBAIKI: chmod 0666 saja, TANPA chown.
 *
 * Bukti logs-6.txt 12:36 WIB (enforcing, permissive=0 di semua AVC):
 * Foto 1 PICTUREZOOM 12:36:19: app tulis 1500x392, TAPI
 *   FileNotFoundException EACCES di WaterMarkTools.java:747
 *   (FileOutputStream.<init>), TANPA AVC water sama sekali
 *   di log (28 AVC semuanya dmabuf/zram/mnld, bukan water).
 *   Jadi EACCES ini DAC, bukan SELinux.
 * Foto 2 PHOTO 12:36:24 (5 detik kemudian): app tulis 748x200,
 *   EACCES yang sama lagi.
 * HAL di kedua foto BISA buka file (fileHandler=247 dan 309,
 *   bukan -1 lagi — DAC baca sudah beres), tapi:
 *   Foto 1: not equal w=1778 wWidth=1500
 *   Foto 2: not equal w=3486 wWidth=748
 * w HAL = ukuran_file / (tinggi_yang_diharapkan * 1,5).
 * File yang dibaca HAL adalah file LAMA yang tidak pernah
 * berhasil ditimpa app, karena v6 sudah chown file ke
 * system:camera + chmod 0664. App (u0_a389, bukan owner,
 * bukan group camera) hanya dapat other-read, TIDAK dapat
 * write, jadi setiap foto berikutnya gagal tulis dan HAL
 * selalu membaca file lama dengan ukuran lama — itulah
 * glitch garis hijau/putih saat ganti resolusi/MP.
 * Bukti hitungan: file lama yang sama, tinggi 392 -> w=1778,
 * tinggi 200 -> w=3486 (1778*392/200 = 3485, cocok).
 *
 * v7:
 *   chmod 0666 saja (rw-rw-rw-), TANPA chown, owner tetap app.
 *   - App bisa tulis lagi (owner rw, dan kalaupun owner berubah
 *     ke system, other rw tetap mengizinkan tulis).
 *   - HAL bisa baca (other r).
 *   - FileOutputStream app akan truncate file lama, jadi ukuran
 *     file selalu sesuai resolusi foto saat itu, w HAL = wWidth.
 *   - File lama yang sudah terlanjur system:camera 0664 akan
 *     diubah v7 jadi 0666 saat pertama terlihat, jadi foto
 *     berikutnya langsung bisa tulis lagi tanpa hapus manual.
 * Hapus setelah foto tetap ada, tapi dipersingkat 5 -> 3 detik
 * (HAL encode ~1,6-2 detik setelah tulis, jadi 3 detik aman,
 * dan jendela file stale saat ganti resolusi jadi lebih pendek).
 * TIDAK sentuh folder, TIDAK setxattr, TIDAK bikin file.
 * Izin chmod file (setattr) sudah ada di pri_water.te.
 */

#define MAX_TRACK 64
#define STABLE_NEEDED 3

typedef struct {
    char name[256];
    time_t last_mtime;
    int stable_count;
    int wrote;
    int mode_fixed;
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

/* Perbaiki mode FILE saja, segera, bukan setelah stabil.
   JANGAN chown: chown ke system merusak tulis app berikutnya. */
static void fix_file_mode(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return;
    if ((st.st_mode & 0777) != 0666)
        chmod(path, 0666);
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

                /* Segera perbaiki mode saat pertama terlihat / tulis baru,
                   JANGAN tunggu stabil — HAL baca ~1,6 detik setelah tulis. */
                if (!t->mode_fixed) {
                    fix_file_mode(path);
                    t->mode_fixed = 1;
                }

                if (!t->wrote) {
                    if (t->last_mtime == 0) {
                        t->last_mtime = st.st_mtime;
                    } else if (st.st_mtime != t->last_mtime) {
                        t->wrote = 1;
                        t->last_mtime = st.st_mtime;
                        t->stable_count = 0;
                        t->mode_fixed = 0;
                        fix_file_mode(path);
                        t->mode_fixed = 1;
                    }
                } else {
                    if (st.st_mtime != t->last_mtime) {
                        t->last_mtime = st.st_mtime;
                        t->stable_count = 0;
                        t->mode_fixed = 0;
                        fix_file_mode(path);
                        t->mode_fixed = 1;
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
