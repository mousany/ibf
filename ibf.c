#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0
#else
#include <unistd.h>
#endif

#define IBF_VERSION_MAJOR 0
#define IBF_VERSION_MINOR 1
#define IBF_VERSION_PATCH 0

/**
 * @brief Run IBF interactively in the console.
 */
void run_console() {
  fprintf(stderr, "IBF %d.%d.%d (tags/v%d.%d.%d, %s, %s)", IBF_VERSION_MAJOR,
          IBF_VERSION_MINOR, IBF_VERSION_PATCH, IBF_VERSION_MAJOR,
          IBF_VERSION_MINOR, IBF_VERSION_PATCH, __DATE__, __TIME__);
}

/**
 * @brief Run IBF from a file.
 * @param file The file to run.
 */
void run_file(FILE *file) {}

/**
 * @brief Run IBF from a command.
 * @param command The command to run.
 */
void run_command(char *command) {}

/**
 * @brief Print the version of IBF.
 */
void print_version() {
  /* Version format: MAJOR.MINOR.PATCH */
  fprintf(stderr, "IBF %d.%d.%d\n", IBF_VERSION_MAJOR, IBF_VERSION_MINOR,
          IBF_VERSION_PATCH);
}

/**
 * @brief Print the usage of IBF.
 */
void print_usage() {
  fprintf(stderr, "usage: ibf [options] ... [-c cmd | file | -] [arg] ...\n");
  fprintf(stderr, "Try `ibf -h` for more information.\n");
}

/**
 * @brief Print the help of IBF.
 */
void print_help() {}

/**
 * @brief Options for IBF.
 * Version: -v, --version. Print the version of the program.
 * Help: -h, --help. Print the help of the program.
 * Command: -c, --command. Run the command from the command line.
 */
static struct option long_options[] = {{"version", no_argument, 0, 'v'},
                                       {"help", no_argument, 0, 'h'},
                                       {"command", required_argument, 0, 'c'},
                                       {0, 0, 0, 0}};

int main(int argc, char *argv[]) {
  while (true) {
    int option_index = 0;
    /* Parse the options. */
    int c = getopt_long(argc, argv, ":vhc:", long_options, &option_index);
    if (c == -1) {
      break;
    }
    switch (c) {
      case 0: /* Long option. */
        if (long_options[option_index].flag != 0) {
          break;
        }
        break;
      case 'v': /* Version. */
        print_version();
        return EXIT_SUCCESS;
      case 'h': /* Help. */
        print_help();
        return EXIT_SUCCESS;
      case 'c': /* Command. */
        run_command(optarg);
        return EXIT_SUCCESS;
      case '?': /* Unknown option. */
        fprintf(stderr, "Unknown option %s\n", argv[optind - 1]);
        print_usage();
        return EXIT_FAILURE;
      case ':': /* Missing argument. */
        fprintf(stderr, "Argument expected for the %s option\n",
                argv[optind - 1]);
        print_usage();
        return EXIT_FAILURE;
      default:
        abort();
    }
  }
  /* If there is no argument, run interactively in the console or from a file.
   */
  if (argc > 1) {
    /* Run from a file. */
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
      fprintf(stderr, "%s: Cannot open file '%s': [Errno %d] %s\n", argv[0],
              argv[1], errno, strerror(errno));
      return EXIT_FAILURE;
    }
    run_file(file);
    fclose(file);
  } else {
    /* Run interactively in the console. */
    if (isatty(STDIN_FILENO)) {
      run_console();
    } else {
      run_file(stdin);
    }
  }
  return EXIT_SUCCESS;
}
