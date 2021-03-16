#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static unsigned DELAY = 5;
static char *FORMAT = "%A %R";

#define ERROR(status, ...) {      \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(status);                 \
}

static noreturn void
usage(char *name)
{
    ERROR(
        1,
        "usage : %s [-r <delay>] [-f <format>]\n"
        "\n"
        "options :\n"
        "    -r <delay>     set delay between refreshes to <delay> (default : %u)\n"
        "    -f <format>    set time format string to <format> (default : %s)\n",
        basename(name), DELAY, FORMAT);
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

static noreturn void
subscribe_clock(const char *format)
{
    bool flag = 0;

    char output[2][LINE_MAX] = {0};

    for (;;) {
        time_t time_value;
        struct tm time_struct;

        time_value = time(0);
        localtime_r(&time_value, &time_struct);

        if (!(strftime(output[flag], sizeof(*output), format, &time_struct)))
            ERROR(1, "error : failed to format current time\n");

        /* print only if output buffer has changed */
        if (strncmp(output[!flag], output[flag], sizeof(*output)) != 0) {
            puts(output[flag]);
            fflush(stdout);
            flag ^= 1;
        }

        sleep(DELAY);
    }
}

int
main(int argc, char **argv)
{
    {
        int arg;

        while ((arg = getopt(argc, argv, ":r:f:")) != -1)
            switch (arg) {
                case 'r':
                    errno = 0;
                    DELAY = _strtou(optarg);

                    if (errno != 0)
                        ERROR(1, "error : '%s' invalid parameter\n", optarg);

                    break;
                case 'f':
                    FORMAT = optarg;
                    break;
                default :
                    usage(argv[0]);
            }
    }

    /* handle mismatched parameters */
    if (optind < argc)
        usage(argv[0]);

    subscribe_clock(FORMAT);
}
