#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ERROR(status, ...) {      \
    fprintf(stderr, __VA_ARGS__); \
                                  \
    exit(status);                 \
}

static unsigned DELAY = 5;
static char *FORMAT = "%A %R";

static noreturn void
usage(char *name)
{
    ERROR(
        1,
        "usage : %s [options]                                     \n"
        "                                                         \n"
        "options :                                                \n"
        "    -f <format>    use <format> for time                 \n"
        "    -d <delay>     check for update every <delay> seconds\n",
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

static noreturn void
watch_clock(const char *format, unsigned delay)
{
    int flag = 0;

    char output[2][LINE_MAX] = {{0}};

    for (;;) {
        time_t time_value;
        time_value = time(0);

        struct tm time_struct;
        localtime_r(&time_value, &time_struct);

        if (!strftime(output[flag], sizeof(*output), format, &time_struct))
            ERROR(1, "error : failed to format current time\n")

        if (strncmp(output[flag], output[!flag], sizeof(*output))) {
            puts(output[flag]);
            fflush(stdout);
            flag ^= 1;
        }

        sleep(delay);
    }
}

int
main(int argc, char **argv)
{
    int arg;

    while ((arg = getopt(argc, argv, ":f:d:")) != -1)
        switch (arg) {
            case 'f':
                FORMAT = optarg;

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

    watch_clock(FORMAT, DELAY);
}
