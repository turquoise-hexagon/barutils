#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

#define ERROR(status, ...) {      \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(status);                 \
}

static unsigned DELAY = 5;
static char *PATH = "/sys/class/power_supply";

struct battery {
    char name[NAME_MAX];
    char charge_path[PATH_MAX];
    char status_path[PATH_MAX];
    char charge[LINE_MAX];
    char status[LINE_MAX];
};

static noreturn void
usage(char *name)
{
    ERROR(
        1,
        "usage : %s [options]                                     \n"
        "                                                         \n"
        "options :                                                \n"
        "    -p <path>     look for battery files in <path>       \n"
        "    -d <delay>    check for updates every <delay> seconds\n",
        basename(name)
    )
}

static unsigned
strtou(const char *str)
{
    long n;

    errno = 0;
    char *ptr;

    if ((n = strtol(str, &ptr, 10)) < 0 || errno != 0 || *ptr != 0) {
        errno = EINVAL;

        return 0;
    }

    return (unsigned)n;
}

static struct battery *
get_batteries(const char *path, size_t *len)
{
    struct battery *batteries;

    size_t allocated = 2;
    size_t assigned  = 0;

    if (!(batteries = malloc(allocated * sizeof(*batteries))))
        ERROR(1, "error : failed to allocate '%lu' bytes of memory\n",
            allocated * sizeof(*batteries))

    DIR *batteries_dir;

    if (!(batteries_dir = opendir(PATH)))
        ERROR(1, "error : failed to open '%s'\n", path)

    struct dirent *entry;

    while ((entry = readdir(batteries_dir)))
        if (!strncmp(entry->d_name, "BAT", 3)) {
            struct battery *cur = &batteries[assigned];

            strncpy(cur->name, entry->d_name, sizeof(cur->name) - 1);

            /* ensure null termination */
            cur->name[sizeof(cur->name) - 1] = 0;

            if (snprintf(cur->charge_path, sizeof(cur->charge_path),
                    "%s/%s/%s", path, entry->d_name, "capacity") < 0)
                ERROR(1, "error : failed to build path to battery files\n")

            if (snprintf(cur->status_path, sizeof(cur->status_path),
                    "%s/%s/%s", path, entry->d_name, "status"  ) < 0)
                ERROR(1, "error : failed to build path to battery files\n")

            cur->charge[0] = 0;
            cur->status[0] = 0;

            /* reallocate if necessary */
            ++assigned;

            if (allocated == assigned) {
                allocated = allocated * 3 / 2;

                if (!(batteries = realloc(batteries, allocated * sizeof(*batteries))))
                    ERROR(1, "error : failed to allocate '%lu' bytes of memory\n",
                        allocated * sizeof(*batteries))
            }
        }

    if (closedir(batteries_dir))
        ERROR(1, "error : failed to close '%s'\n", path)

    if (assigned == 0)
        ERROR(1, "error : no batteries found in '%s'\n", path)

    *len = assigned;

    return batteries;
}

static int
update_field(char *dest, char *path, size_t len)
{
    FILE *file;

    if (!(file = fopen(path, "r")))
        ERROR(1, "error : failed to open '%s'\n", path)

    char content[LINE_MAX] = {0};

    if (!fgets(content, sizeof(content), file))
        ERROR(1, "error : failed to get content from '%s'\n", path)

    /* fix string */
    content[strnlen(content, sizeof(content)) - 1] = 0;

    if (fclose(file))
        ERROR(1, "error : failed to close '%s'\n", path)

    if (strncmp(dest, content, len)) {
        strncpy(dest, content, len - 1);

        /* ensure null termination */
        dest[len - 1] = 0;

        return 1;
    }

    return 0;
}

static int
update_batteries(struct battery *batteries, size_t len)
{
    int flag = 0;

    for (size_t i = 0; i < len; ++i) {
        struct battery *cur = &batteries[i];

        if (update_field(cur->charge, cur->charge_path,
                sizeof(cur->charge))) flag = 1;

        if (update_field(cur->status, cur->status_path,
                sizeof(cur->status))) flag = 1;
    }

    return flag;
}

static int
compare_batteries(struct battery *a, struct battery *b, size_t len_a, size_t len_b)
{
    if (len_a != len_b)
        return 0;

    for (size_t i = 0; i < len_a; ++i) {
        struct battery *cur_a = &a[i];
        struct battery *cur_b = &b[i];

        if (strncmp(cur_a->name, cur_b->name, sizeof(cur_a->name)))
            return 0;
    }

    return 1;
}

static noreturn void
watch_batteries(char *path, unsigned delay)
{
    size_t len;
    struct battery *batteries;

    batteries = get_batteries(path, &len);

    for (;;) {
        size_t len_tmp;
        struct battery *batteries_tmp;

        batteries_tmp = get_batteries(path, &len_tmp);

        /* swap lists if batteries layout changed */
        if (!compare_batteries(batteries, batteries_tmp, len, len_tmp)) {
            free(batteries);

            batteries = batteries_tmp;
            len = len_tmp;
        }
        else
            free(batteries_tmp);

        if (update_batteries(batteries, len)) {
            for (size_t i = 0; i < len; ++i) {
                struct battery *cur = &batteries[i];

                printf("%s %s %s:", cur->name, cur->charge, cur->status);
            }

            putchar('\n');
            fflush(stdout);
        }

        sleep(delay);
    }
}

int
main(int argc, char **argv)
{
    int arg;

    while ((arg = getopt(argc, argv, ":p:d:")) != -1)
        switch (arg) {
            case 'p':
                PATH = optarg;

                break;
            case 'd':
                errno = 0;
                DELAY = strtou(optarg);

                if (errno != 0)
                    ERROR(1, "error : '%s' invalid parameter\n", optarg)

                break;
            default :
                usage(argv[0]);
        }

    /* handle mismatched parameters */
    if (optind < argc)
        usage(argv[0]);

    watch_batteries(PATH, DELAY);
}
