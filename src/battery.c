#include <dirent.h>
#include <errno.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

static unsigned DELAY = 5;

static const char PATH[] = "/sys/class/power_supply";

#define ERROR(status, ...) {      \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(status);                 \
}

struct battery {
    char name[NAME_MAX];

    char charge_path[PATH_MAX];
    char status_path[PATH_MAX];

    bool flag;

    char output[2][LINE_MAX];
};

static noreturn void
usage(char *name)
{
    ERROR(1, "usage : %s [-r <delay> (default : %u)]\n", basename(name), DELAY);
}

static unsigned
_strtou(const char *str)
{
    errno = 0;
    long tmp;

    {
        char *ptr;

        /* accept only valid unsigned integers */
        if ((tmp = strtol(str, &ptr, 10)) < 0 || errno != 0 || *ptr != 0)
            return 0;
    }

    return (unsigned)tmp;
}

static char *
_strncpy(char *dest, const char *src, size_t size)
{
    int tmp;

    /* expensive but reliable */
    if ((tmp = snprintf(dest, size, "%s", src)) < 0)
        return NULL;

    return dest;
}

static char *
get_value_from_file(char *dest, const char *path, size_t size)
{
    FILE *file;

    if (!(file = fopen(path, "r")))
        return NULL;

    if (!fgets(dest, (int)size, file))
        return NULL;

    if (fclose(file))
        return NULL;

    /* fix string */
    dest[strnlen(dest, size) - 1] = 0;

    return dest;
}

static struct battery *
get_batteries(const char *path, size_t *size)
{
    static struct battery *bt;

    {
        size_t alloc  = 2;
        size_t assign = 0;

        if (!(bt = malloc(alloc * sizeof(*bt))))
            ERROR(1, "error : failed to allocate '%lu' bytes of memory",
                alloc * sizeof(*bt));

        DIR *dir;
        struct dirent *ent;

        if (!(dir = opendir(path)))
            ERROR(1, "error : failed to open '%s'\n", path);

        while ((ent = readdir(dir)))
            if (strncmp(ent->d_name, "BAT", 3) == 0) {
                struct battery *tmp = &bt[assign];

                tmp->flag = 0;
                memset(tmp->output[0], 0, sizeof(tmp->output[0]));
                memset(tmp->output[1], 0, sizeof(tmp->output[1]));

                if (!_strncpy(tmp->name, ent->d_name, sizeof(tmp->name)))
                    ERROR(1, "error : failed to get list of batteries\n");

                if (snprintf(tmp->charge_path, sizeof(tmp->charge_path),
                    "%s/%s/capacity", path, ent->d_name) < 0)
                    ERROR(1, "error : failed to get list of batteries\n");

                if (snprintf(tmp->status_path, sizeof(tmp->status_path),
                    "%s/%s/status",   path, ent->d_name) < 0)
                    ERROR(1, "error : failed to get list of batteries\n");

                /* resize buffer if necessary */
                if (++assign == alloc)
                    if (!(bt = realloc(bt, (alloc = alloc * 3 / 2) * sizeof(*bt))))
                        ERROR(1, "error : failed to allocate '%lu' bytes of memory\n",
                            alloc * sizeof(*bt));
            }

        *size = assign;
    }

    return bt;
}

static noreturn void
subscribe_batteries(struct battery *bt, size_t size)
{

    char charge[LINE_MAX] = {0};
    char status[LINE_MAX] = {0};

    for (;;) {
        for (size_t i = 0; i < size; ++i) {
            struct battery *cur = &bt[i];

            bool flag = cur->flag;

            if (!get_value_from_file(charge, cur->charge_path, sizeof(charge)))
                ERROR(1, "error : failed to get content from '%s'\n", cur->charge_path);

            if (!get_value_from_file(status, cur->status_path, sizeof(status)))
                ERROR(1, "error : failed to get content from '%s'\n", cur->status_path);

            if (snprintf(cur->output[flag], sizeof(cur->output[flag]),
                "%s %s %s", cur->name, status, charge) < 0)
                ERROR(1, "error : failed to format output\n");

            /* print only if output buffer has changed */
            if (strncmp(cur->output[!flag], cur->output[flag], sizeof(cur->output[!flag])) != 0) {
                puts(cur->output[flag]);
                fflush(stdout);
                cur->flag ^= 1;
            }
        }
        
        sleep(DELAY);
    }
}

int
main(int argc, char **argv)
{
    {
        int arg;

        while ((arg = getopt(argc, argv, ":r:")) != -1)
            switch (arg) {
                case 'r':
                    errno = 0;
                    DELAY = _strtou(optarg);

                    if (errno != 0)
                        ERROR(1, "error : '%s' invalid parameter\n", optarg);

                    break;
                default :
                    usage(argv[0]);
            }
    }

    /* handle mismatched parameters */
    if (optind < argc)
        usage(argv[0]);

    size_t size;
    struct battery *bt;

    bt = get_batteries(PATH, &size);
    subscribe_batteries(bt, size);
}
