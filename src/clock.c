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

static unsigned delay = 5;
static char format[LINE_MAX] = "%A %R";

static noreturn void
usage(char *name)
{
    fprintf(
        stderr,
        "usage : %s [-f <string>] [-r <number>]\n"
        "\n"
        "options :\n"
        "    -f <string>    set format string to <string> (default : %s)\n"
        "    -r <number>    set delay between refreshes to <number> (default : %u)\n",
        basename(name),
        format,
        delay);

    exit(1);
}

static unsigned
convert_to_number(const char *str)
{
    errno = 0;
    long num;

    {
        char *ptr;

        if ((num = strtol(str, &ptr, 10)) < 0 || errno != 0 || *ptr != 0) {
            fprintf(stderr, "error : '%s' isn't a valid delay\n", str);

            exit(1);
        }
    }

    return (unsigned)num;
}

int
main(int argc, char **argv)
{
    for (int arg; (arg = getopt(argc, argv, ":f:r:")) != -1;)
        switch (arg) {
            case 'f': delay = convert_to_number(optarg);     break;
            case 'r': strncpy(format, optarg, LINE_MAX - 1); break;
            default :
                usage(argv[0]);
        }

    if (optind < argc)
        usage(argv[0]);

    bool flag = 0;
    char output[2][1024] = {{0}};

    for (;;) {
        time_t time_value;
        struct tm *time_struct;

        time_value = time(0);
        time_struct = localtime(&time_value);

        if (! strftime(output[flag], 1024, format, time_struct)) {
            fprintf(stderr, "error : failed to format current time\n");

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
