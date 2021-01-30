#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>

static unsigned delay = 5;

static const char PATH[PATH_MAX] = "/sys/class/power_supply";

#define OUTPUT_MAX 128

#define ERROR(code, ...) {        \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(code);                   \
}

struct battery {
    char name[NAME_MAX + 1];

    char capacity_path[PATH_MAX];
    char   status_path[PATH_MAX];

    bool flag;
    char output[2][OUTPUT_MAX];
};

static noreturn void
usage(const char *name)
{
    ERROR(1, "usage : %s [-r <delay> (default : %u)]\n", name, delay)
}

static void *
allocate(size_t size)
{
    void *ptr;

    if (! (ptr = calloc(1, size)))
        ERROR(1, "error : failed to allocate '%lu' bytes of memory\n", size)

    return ptr;
}

static void *
reallocate(void *old, size_t size)
{
    void *new;

    if (! (new = realloc(old, size)))
        ERROR(1, "error : failed to allocate '%lu' bytes of memory\n", size)

    return new;
}

static unsigned
convert(const char *str)
{
    errno = 0;
    long num;

    {
        char *ptr;

        if ((num = strtol(str, &ptr, 10)) < 0 || *ptr != 0 || errno != 0)
            ERROR(1, "error : '%s' isn't a valid delay\n", str)
    }

    return (unsigned)num;
}

static DIR *
open_dir(const char *path)
{
    DIR *dir;

    if (! (dir = opendir(path)))
        ERROR(1, "error : failed to open '%s'\n", path)

    return dir;
}

static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file;

    if (! (file = fopen(path, mode)))
        ERROR(1, "error : failed to open '%s'\n", path)

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) != 0)
        ERROR(1, "error : failed to close '%s'\n", path)
}

static void
build_path(char *dest, const char *path, const char *dir, const char *file, size_t size)
{
    if (snprintf(dest, size, "%s/%s/%s", path, dir, file) < 0)
        ERROR(1, "error : failed to build path '%s/%s/%s'\n", path, dir, file)
}

static struct battery *
get_batteries(size_t *size)
{
    static struct battery *batteries;

    {
        size_t allocated = 2;
        size_t  assigned = 0;

        batteries = allocate(allocated * sizeof(*batteries));

        DIR *dir;
        struct dirent *ent;

        dir = open_dir(PATH);

        while ((ent = readdir(dir)))
            if (strncmp(ent->d_name, "BAT", 3 * sizeof(*ent->d_name)) == 0) {
                strncpy(batteries[assigned].name, ent->d_name, NAME_MAX + 1);

                build_path(batteries[assigned].capacity_path, PATH, ent->d_name, "capacity", PATH_MAX);
                build_path(  batteries[assigned].status_path, PATH, ent->d_name,   "status", PATH_MAX);

                batteries[assigned].flag = 0;

                /* reallocate memory if necessary */
                if (++assigned == allocated)
                    batteries = reallocate(batteries, (allocated = allocated * 3 / 2) * sizeof(*batteries));
            }

        *size = assigned;
    }

    return batteries;
}

static void
get_content(const char *path, char *output, size_t size)
{
    FILE *file;

    file = open_file(path, "r");

    if (! fgets(output, (int)size, file))
        ERROR(1, "error : failed to get content from '%s'\n", path)

    /* fix string */
    output[strnlen(output, size) - 1] = 0;

    close_file(path, file);
}

static noreturn void
subscribe(struct battery *batteries, size_t size)
{
    char   status[OUTPUT_MAX] = {0};
    char capacity[OUTPUT_MAX] = {0};

    for (;;) {
        for (size_t i = 0; i < size; ++i) {
            get_content(batteries[i].capacity_path, capacity, OUTPUT_MAX);
            get_content(  batteries[i].status_path,   status, OUTPUT_MAX);

            if (snprintf(batteries[i].output[batteries[i].flag], OUTPUT_MAX, "%s %s", status, capacity) < 0)
                ERROR(1, "error : failed to build output\n")

            /* only output if content has changed */
            if (strncmp(batteries[i].output[batteries[i].flag],
                batteries[i].output[! batteries[i].flag], OUTPUT_MAX) != 0) {
                printf("%s %s\n", batteries[i].name, batteries[i].output[batteries[i].flag]);
                batteries[i].flag ^= 1;
                fflush(stdout);
            }
        }

        sleep(delay);
    }
}

int
main(int argc, char **argv)
{
    const char *name = basename(argv[0]);

    {
        int arg;

        while ((arg = getopt(argc, argv, ":r:")) != -1)
            switch (arg) {
                case 'r':
                    delay = convert(optarg);
                    break;
                default:
                    usage(name);
            }
    }

    if (optind < argc)
        usage(name);

    size_t size;
    struct battery *batteries;

    batteries = get_batteries(&size);
    subscribe(batteries, size);
}
