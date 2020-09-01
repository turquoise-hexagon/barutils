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
static char sys_path[PATH_MAX] = "/sys/class/power_supply/BAT0/";

static noreturn void
usage(char *name)
{
    fprintf(
        stderr,
        "usage : %s [-p <path>] [-r <number>]\n"
        "\n"
        "options :\n"
        "    -p <path>      set path to brightness files to <path> (default : %s)\n"
        "    -r <number>    set delay between refreshes to <number> (default : %u)\n",
        basename(name),
        sys_path,
        delay);

    exit(1);
}

static bool
convert_to_number(const char *str, unsigned *num)
{
    errno = 0;

    char *ptr;
    long tmp;

    if ((tmp = strtol(str, &ptr, 10)) < 0 || errno != 0 || *ptr != 0)
        return 0;

    *num = (unsigned)tmp;

    return 1;
}

static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file;

    if (! (file = fopen(path, mode))) {
        fprintf(stderr, "error : failed to open '%s'\n", path);

        exit(1);
    }

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) != 0) {
        fprintf(stderr, "error : failed to close '%s'\n", path);

        exit(1);
    }
}

static void
get_value_from_file(const char *path, char output[LINE_MAX])
{
    FILE *file;

    file = open_file(path, "r");

    if (! (fgets(output, LINE_MAX, file))) {
        fprintf(stderr, "error : failed to get content from '%s'\n", path);

        exit(1);
    }

    close_file(path, file);

    /* fix string */
    output[strnlen(output, LINE_MAX) - 1] = 0;
}

int
main(int argc, char **argv)
{
    for (int arg; (arg = getopt(argc, argv, ":p:r:")) != -1;)
        switch (arg) {
            case 'p':
                strncpy(sys_path, optarg, PATH_MAX - 1);

                break;
            case 'r':;
                if (! convert_to_number(optarg, &delay)) {
                    fprintf(stderr, "error : '%s' isn't a valid delay\n", optarg);

                    exit(1);
                }

                break;
            default :
                usage(argv[0]);
        }

    char status_path[PATH_MAX]   = {0};
    char capacity_path[PATH_MAX] = {0};

    if (snprintf(status_path, PATH_MAX, "%sstatus", sys_path) < 0) {
        fprintf(stderr, "error : failed to build path to status file\n");

        exit(1);
    }

    if (snprintf(capacity_path, PATH_MAX, "%scapacity", sys_path) < 0) {
        fprintf(stderr, "error : failed to build path to capacity file\n");

        exit(1);
    }

    bool flag = 0;
    char output[2][1024] = {{0}};

    char status[LINE_MAX]   = {0};
    char capacity[LINE_MAX] = {0};

    for (;;) {
        get_value_from_file(status_path,   status);
        get_value_from_file(capacity_path, capacity);

        unsigned tmp;

        if (! convert_to_number(capacity, &tmp)) {
            fprintf(stderr, "error : '%s' invalid capacity\n", capacity);

            exit(1);
        }

        if (snprintf(output[flag], 1024, "%u %s", tmp, status) < 0) {
            fprintf(stderr, "error : failed to store output in buffer\n");

            exit(1);
        }

        /* only output if content has changed */
        if (strncmp(output[flag], output[!flag], 1024)) {
            puts(output[flag]);
            fflush(stdout);
            flag ^= 1;
        }

        sleep(delay);
    }
}
