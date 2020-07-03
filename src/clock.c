#include <err.h>
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

static noreturn void
usage(char *name)
{
    fprintf(
        stderr,
        "usage : %s [-s <number>] [-f <string>]\n\n"
        "options : \n"
        "    -f <string>    set format string to <string> (default : %%A %%R)\n"
        "    -s <number>    set delay between refreshes to <number> (default : 5)\n",
        basename(name));

    exit(1);
}

/*
 * helper functions
 */
static unsigned
convert_to_number(const char *str)
{
    errno = 0;

    char *ptr;
    long number;

    if ((number = strtol(str, &ptr, 10)) < 0 || errno != 0 || *ptr != 0)
        errx(1, "'%s' isn't a valid positive integer", str);

    return (unsigned)number;
}

/*
 * main functions
 */
int
main(int argc, char **argv)
{
    /* default values */
    unsigned delay = 5;
    char format[LINE_MAX] = "%A %R";

    /* argument parsing */
    for (int arg; (arg = getopt(argc, argv, ":s:f:")) != -1;)
        switch (arg) {
            case 's': delay = convert_to_number(optarg);     break;
            case 'f': strncpy(format, optarg, LINE_MAX - 1); break;
            default :
                usage(argv[0]);
        }

    if (optind < argc)
        usage(argv[0]);

    /* main loop */
    bool flag = 0;
    char output[2][LINE_MAX] = {{0}};

    time_t time_value;
    struct tm *time_struct;

    for (;;) {
        time_value = time(0);
        time_struct = localtime(&time_value);

        if (strftime(output[flag], LINE_MAX, format, time_struct) == 0)
            errx(1, "failed to format current time");

        /* don't print anything if there was no updates */
        if (strncmp(output[flag], output[!flag], LINE_MAX) != 0) {
            puts(output[flag]);
            fflush(stdout);
            flag ^= 1;
        }
        
        sleep(delay);
    }
}
