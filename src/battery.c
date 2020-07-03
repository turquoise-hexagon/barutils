#include <err.h>
#include <libgen.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

static noreturn void
usage(char *name)
{
    fprintf(
        stderr,
        "usage : %s [-s <number>] [-p <string>]\n\n"
        "options :\n"
        "    -p <string>    set path to brightness directory to <string> (default : /sys/class/power_supply/BAT0)\n"
        "    -s <number>    set delay between refreshes to <number> (default : 5)\n",
        basename(name));

    exit(1);
}

/*
 * helper functions
 */
static FILE *
open_file(const char *path, const char *mode)
{
    FILE *file;

    if ((file = fopen(path, mode)) == NULL)
        errx(1, "failed to open '%s'", path);

    return file;
}

static void
close_file(const char *path, FILE *file)
{
    if (fclose(file) == EOF)
        errx(1, "faile to close '%s'", path);
}

static void
get_content(const char *path, char *content)
{
    FILE *file;

    file = open_file(path, "r");

    if (fgets(content, LINE_MAX, file) == NULL)
        errx(1, "failed to get content from '%s'", path);

    /* fix string */
    content[strnlen(content, LINE_MAX) - 1] = 0;

    close_file(path, file);
}

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
    char path[PATH_MAX] = "/sys/class/power_supply/BAT0";

    /* argument parsing */
    for (int arg; (arg = getopt(argc, argv, ":s:p:")) != -1;)
        switch (arg) {
            case 's': delay = convert_to_number(optarg);   break;
            case 'p': strncpy(path, optarg, PATH_MAX - 1); break;
            default :
                usage(argv[0]);
        }

    if (optind < argc)
        usage(argv[0]);

    /* obtain path to files */
    char status_path[PATH_MAX]   = {0};
    char capacity_path[PATH_MAX] = {0};

    if (snprintf(status_path, PATH_MAX, "%s/status", path) < 0)
        errx(1, "failed to build path to battery status file");

    if (snprintf(capacity_path, PATH_MAX, "%s/capacity", path) < 0)
        errx(1, "failed to build path to battery capacity file");

    /* main loop */
    bool flag = 0;
    char output[2][LINE_MAX] = {{0}};

    char status[LINE_MAX]   = {0};
    char capacity[LINE_MAX] = {0};

    for (;;) {
        get_content(status_path,     status);
        get_content(capacity_path, capacity);

        if (snprintf(output[flag], LINE_MAX, "%u %s",
               convert_to_number(capacity), status) < 0)
            errx(1, "failed to store command output");

        /* don't print anything if there was no updates */
        if (strncmp(output[flag], output[!flag], LINE_MAX) != 0) {
            puts(output[flag]);
            fflush(stdout);
            flag ^= 1;
        }
    
        sleep(delay);
    }
}
