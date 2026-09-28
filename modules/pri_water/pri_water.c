#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <pwd.h>
#include <grp.h>

int main() {
    const char *dir = "/data/vendor/.water";
    const char *label = "u:object_r:mtk_water_data_file:s0";

    struct passwd *pw = getpwnam("system");
    struct group  *gr = getgrnam("camera");
    uid_t uid = pw ? pw->pw_uid : 1000;
    gid_t gid = gr ? gr->gr_gid : 1006;

    while (1) {
        DIR *d = opendir(dir);
        if (d) {
            struct dirent *ent;
            char path[512];
            while ((ent = readdir(d)) != NULL) {
                if (ent->d_name[0] == '.') continue;
                snprintf(path, sizeof(path), "%s/%s", dir, ent->d_name);
                chmod(path, 0664);
                chown(path, uid, gid);
                setxattr(path, "security.selinux", label, strlen(label) + 1, 0);
            }
            closedir(d);
        }
        sleep(1);
    }
    return 0;
}
