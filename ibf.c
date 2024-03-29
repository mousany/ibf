#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BRAINFUCK_TOKEN_PLUS '+'
#define BRAINFUCK_TOKEN_MINUS '-'
#define BRAINFUCK_TOKEN_PREVIOUS '<'
#define BRAINFUCK_TOKEN_NEXT '>'
#define BRAINFUCK_TOKEN_OUTPUT '.'
#define BRAINFUCK_TOKEN_INPUT ','
#define BRAINFUCK_TOKEN_LOOP_START '['
#define BRAINFUCK_TOKEN_LOOP_END ']'

#define BRAINFUCK_MEMORY_BUFFER_SIZE 30000
#define BRAINFUCK_LOOP_BUFFER_SIZE 100000
#define BRAINFUCK_MAX_LINE_LENGTH 100000
#define BRAINFUCK_MAX_LOOP_DEPTH 65536

/**
 * @brief The input handler of IBF.
 * @return The character to input.
 */
typedef uint8_t (*brainfuck_input_handler)();

/**
 * @brief The output handler of IBF.
 * @param c The character to output.
 */
typedef void (*brainfuck_output_handler)(uint8_t c);

/**
 * @brief The state of IBF.
 */
struct brainfuck_state {
  uint8_t *memory_buffer; /* The memory buffer. */
  size_t memory_pointer;  /* The memory pointer. */
  char *loop_buffer;      /* The loop buffer. */
  size_t loop_size;       /* The loop size. */
  size_t unmatched_depth; /* The unmatched depth of loop. */
};

/**
 * @brief The context of IBF.
 */
struct brainfuck_context {
  struct brainfuck_state *state;           /* The state of IBF. */
  brainfuck_input_handler input_handler;   /* The input handler of IBF. */
  brainfuck_output_handler output_handler; /* The output handler of IBF. */
};

/**
 * @brief Create a new state of IBF.
 * @return The new state of IBF.
 */
struct brainfuck_state *brainfuck_state_new() {
  struct brainfuck_state *state = malloc(sizeof(struct brainfuck_state));
  state->memory_buffer = calloc(BRAINFUCK_MEMORY_BUFFER_SIZE, sizeof(uint8_t));
  state->memory_pointer = 0;
  state->loop_buffer = calloc(BRAINFUCK_LOOP_BUFFER_SIZE, sizeof(char));
  state->loop_size = 0;
  state->unmatched_depth = 0;
  return state;
}

/**
 * @brief Free the state of IBF.
 * @param state The state of IBF.
 */
void brainfuck_state_free(struct brainfuck_state *state) {
  if (state == NULL) {
    return;
  }
  free(state->memory_buffer);
  free(state->loop_buffer);
  free(state);
}

/**
 * @brief Create a new context of IBF.
 * @param input_handler The input handler of IBF.
 * @param output_handler The output handler of IBF.
 * @return The new context of IBF.
 */
struct brainfuck_context *brainfuck_context_new(
    brainfuck_input_handler input_handler,
    brainfuck_output_handler output_handler) {
  struct brainfuck_context *context = malloc(sizeof(struct brainfuck_context));
  context->state = brainfuck_state_new();
  context->output_handler = output_handler;
  context->input_handler = input_handler;
  return context;
}

/**
 * @brief Free the context of IBF.
 * @param context The context of IBF.
 */
void brainfuck_context_free(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  brainfuck_state_free(context->state);
  free(context);
}

void print_error_unmatched_loop_end() {
  fprintf(stderr, "SyntaxError: unmatched ']'.\n");
}

void print_error_unmatched_loop_start() {
  fprintf(stderr, "SyntaxError: unmatched '['.\n");
}

void print_error_max_loop_depth() {
  fprintf(stderr, "LoopError: maximum loop depth exceeded.\n");
}

void print_error_max_line_length() {
  fprintf(stderr, "InputError: max line length exceeded.\n");
}

void print_error_max_loop_size() {
  fprintf(stderr, "LoopError: maximum loop size exceeded.\n");
}

void print_error_unexpected_eof() {
  fprintf(stderr, "IOError: unexpected EOF.\n");
}

/**
 * @brief Execute the plus instruction `+` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_plus(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  context->state->memory_buffer[context->state->memory_pointer] += 1;
}

/**
 * @brief Execute the minus instruction `-` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_minus(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  context->state->memory_buffer[context->state->memory_pointer] -= 1;
}

/**
 * @brief Execute the previous instruction `<` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_previous(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  if (context->state->memory_pointer == 0) {
    context->state->memory_pointer = BRAINFUCK_MEMORY_BUFFER_SIZE - 1;
  } else {
    context->state->memory_pointer -= 1;
  }
}

/**
 * @brief Execute the next instruction `>` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_next(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  if (context->state->memory_pointer == BRAINFUCK_MEMORY_BUFFER_SIZE - 1) {
    context->state->memory_pointer = 0;
  } else {
    context->state->memory_pointer += 1;
  }
}

/**
 * @brief Execute the input instruction `,` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_input(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  context->state->memory_buffer[context->state->memory_pointer] =
      context->input_handler();
}

/**
 * @brief Execute the output instruction `.` of brainfuck.
 * @param context The context of IBF.
 */
void brainfuck_execute_output(struct brainfuck_context *context) {
  if (context == NULL) {
    return;
  }
  context->output_handler(
      context->state->memory_buffer[context->state->memory_pointer]);
}

bool brainfuck_loop_enque(struct brainfuck_context *context, char c) {
  if (context == NULL) {
    return false;
  }
  if (context->state->loop_size == BRAINFUCK_LOOP_BUFFER_SIZE) {
    print_error_max_loop_size();
    return false;
  }
  context->state->loop_buffer[context->state->loop_size] = c;
  context->state->loop_size += 1;
  return true;
}

bool brainfuck_loop_increase_unmatched(struct brainfuck_context *context) {
  if (context == NULL) {
    return false;
  }
  if (context->state->unmatched_depth == BRAINFUCK_MAX_LOOP_DEPTH) {
    print_error_max_loop_depth();
    return false;
  }
  context->state->unmatched_depth += 1;
  return true;
}

void brainfuck_loop_execute(struct brainfuck_context *context) {
  if (context == NULL || context->state->loop_size == 0) {
    return;
  }
  if (context->state->memory_buffer[context->state->memory_pointer] == 0) {
    context->state->loop_size = 0;
    return;
  }
  size_t *loop_stack = malloc(sizeof(size_t) * BRAINFUCK_MAX_LOOP_DEPTH);
  size_t loop_stack_size = 0;
  size_t execute_pointer = 0;
  while (execute_pointer < context->state->loop_size) {
    if (context->state->loop_buffer[execute_pointer] ==
        BRAINFUCK_TOKEN_LOOP_START) {
      if (context->state->memory_buffer[context->state->memory_pointer] ==
          0) {  // skip the loop if value is zero.
        size_t unmatched_depth = 1;
        execute_pointer += 1;
        while (unmatched_depth > 0) {
          if (context->state->loop_buffer[execute_pointer] ==
              BRAINFUCK_TOKEN_LOOP_START) {
            unmatched_depth += 1;
          } else if (context->state->loop_buffer[execute_pointer] ==
                     BRAINFUCK_TOKEN_LOOP_END) {
            unmatched_depth -= 1;
          }
          execute_pointer += 1;
        }
      } else {  // if value is not zero, then execute the loop.
        loop_stack[loop_stack_size] = execute_pointer;
        loop_stack_size += 1;
        execute_pointer += 1;
      }
    } else if (context->state->loop_buffer[execute_pointer] ==
               BRAINFUCK_TOKEN_LOOP_END) {
      if (context->state->memory_buffer[context->state->memory_pointer] != 0) {
        execute_pointer = loop_stack[loop_stack_size - 1] + 1;
      } else {
        loop_stack_size -= 1;
        execute_pointer += 1;
      }
    } else {
      switch (context->state->loop_buffer[execute_pointer]) {
        case BRAINFUCK_TOKEN_PLUS:
          brainfuck_execute_plus(context);
          break;
        case BRAINFUCK_TOKEN_MINUS:
          brainfuck_execute_minus(context);
          break;
        case BRAINFUCK_TOKEN_PREVIOUS:
          brainfuck_execute_previous(context);
          break;
        case BRAINFUCK_TOKEN_NEXT:
          brainfuck_execute_next(context);
          break;
        case BRAINFUCK_TOKEN_INPUT:
          brainfuck_execute_input(context);
          break;
        case BRAINFUCK_TOKEN_OUTPUT:
          brainfuck_execute_output(context);
          break;
        default:
          break;
      }
      execute_pointer += 1;
    }
  }
  context->state->loop_size = 0;
  free(loop_stack);
}

/**
 * @brief Excute a line of brainfuck code.
 * @param context The context of IBF.
 * @param src A line of brainfuck code to execute.
 * @return True if the line is executed successfully, false otherwise.
 */
bool brainfuck_main(struct brainfuck_context *context, char *src) {
  if (context == NULL || src == NULL) {
    return false;
  }
  for (size_t i = 0; src[i] != '0'; i += 1) {
    /* If the unmatched depth is 0, we can execute the instruction. */
    if (context->state->unmatched_depth == 0) {
      switch (src[i]) {
        case BRAINFUCK_TOKEN_PLUS:
          brainfuck_execute_plus(context);
          break;
        case BRAINFUCK_TOKEN_MINUS:
          brainfuck_execute_minus(context);
          break;
        case BRAINFUCK_TOKEN_PREVIOUS:
          brainfuck_execute_previous(context);
          break;
        case BRAINFUCK_TOKEN_NEXT:
          brainfuck_execute_next(context);
          break;
        case BRAINFUCK_TOKEN_INPUT:
          brainfuck_execute_input(context);
          break;
        case BRAINFUCK_TOKEN_OUTPUT:
          brainfuck_execute_output(context);
          break;
        case BRAINFUCK_TOKEN_LOOP_START:
          if (!brainfuck_loop_increase_unmatched(context)) {
            return false;
          }
          brainfuck_loop_enque(context, src[i]);
          break;
        case BRAINFUCK_TOKEN_LOOP_END:
          print_error_unmatched_loop_end();
          return false;
        default:
          break;
      }
    } else {
      /* If the unmatched depth is not 0, we can only record the instruction. */
      switch (src[i]) {
        case BRAINFUCK_TOKEN_PLUS:
        case BRAINFUCK_TOKEN_MINUS:
        case BRAINFUCK_TOKEN_PREVIOUS:
        case BRAINFUCK_TOKEN_NEXT:
        case BRAINFUCK_TOKEN_INPUT:
        case BRAINFUCK_TOKEN_OUTPUT:
          brainfuck_loop_enque(context, src[i]);
          break;
        case BRAINFUCK_TOKEN_LOOP_START:
          if (!brainfuck_loop_increase_unmatched(context)) {
            return false;
          }
          brainfuck_loop_enque(context, src[i]);
          break;
        case BRAINFUCK_TOKEN_LOOP_END:
          context->state->unmatched_depth -= 1;
          if (!brainfuck_loop_enque(context, src[i])) {
            return false;
          }
          if (context->state->unmatched_depth == 0) {
            /* If the unmatched depth is 0, we can execute the loop. */
            brainfuck_loop_execute(context);
          }
          break;
        default:
          break;
      }
    }
  }
  return true;
}

/**
 * @brief Read a line ending with `until` from a stream.
 * @param stream The stream to read.
 * @param dest The destination buffer.
 * @param size The size of the destination buffer.
 * @param until The character to end the line.
 * @return True if the line is read successfully.
 */
bool brainfuck_readline_util(FILE *stream, char *dest, size_t size,
                             const char until) {
  if (stream == NULL || dest == NULL || size == 0) {
    return false;
  }
  char ch = fgetc(stream);
  size_t i = 0;
  while (true) {
    if (ch == until || ch == EOF || feof(stream)) {
      break;
    }
    if (i >= size - 1) { /* Leave a space for '\0'. */
      print_error_max_line_length();
      return false;
    } else {
      dest[i] = ch;
      i += 1;
    }
    ch = fgetc(stream);
  }
  dest[i] = '\0';
  return true;
}

#if defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32) && !defined(__CYGWIN__)
#include <io.h>
#define isatty _isatty
#define STDIN_FILENO 0
#else
#include <stdio_ext.h>
#include <termios.h>
#include <unistd.h>
#endif

/**
 * @brief Flush the stdin buffer.
 */
void stdin_flush() {
#if defined(WIN32) || defined(_WIN32) || \
    defined(__WIN32) && !defined(__CYGWIN__)
  fflush(stdin);
#else
  __fpurge(stdin);
  tcflush(STDIN_FILENO, TCIFLUSH);
#endif
}

#if defined(__GNUC__)
#define COMPILER_NAME "GCC"
#define COMPILER_VERSION __VERSION__
#elif defined(_MSC_VER)
#define COMPILER_NAME "MSVC"
#define COMPILER_VERSION _MSC_VER
#elif defined(__clang__)
#define COMPILER_NAME "Clang"
#define COMPILER_VERSION __clang_version__
#else
#define COMPILER_NAME "Unknown"
#define COMPILER_VERSION "Unknown"
#endif

#if defined(__linux__)
#define OS_NAME "linux"
#elif defined(__APPLE__)
#define OS_NAME "macos"
#elif defined(_WIN32) || defined(_WIN64)
#define OS_NAME "win32"
#else
#define OS_NAME "unknown"
#endif

#define IBF_VERSION_MAJOR 0
#define IBF_VERSION_MINOR 1
#define IBF_VERSION_PATCH 0

uint8_t brainfuck_input_handler_stdin() {
  char ch = getchar();
  if (ch == EOF) {
    // print_error_unexpected_eof();
    exit(EXIT_FAILURE);
  }
  return ch;
}

void brainfuck_output_handler_stdout(uint8_t c) {
  if (putchar(c) == EOF) {
    // print_error_unexpected_eof();
    exit(EXIT_FAILURE);
  }
}

void console_print_help() {
  fprintf(stderr, "Welcome to IBF %d.%d.%d!\n", IBF_VERSION_MAJOR,
          IBF_VERSION_MINOR, IBF_VERSION_PATCH);
  fprintf(stderr, "\n");
  fprintf(
      stderr,
      "Brainfuck is an esoteric programming language created in 1993 "
      "by Urban \nMuller. And it is the smallest Turing-complete language.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Instructions of Brainfuck:\n");
  fprintf(stderr,
          ">  Increment the data pointer (to point to the next cell to the "
          "right).\n");
  fprintf(
      stderr,
      "Decrement the data pointer (to point to the next cell to the left).\n");
  fprintf(stderr,
          "+  Increment (increase by one) the byte at the data pointer.\n");
  fprintf(stderr,
          "-  Decrement (decrease by one) the byte at the data pointer.\n");
  fprintf(stderr, ".  Output the byte at the data pointer.\n");
  fprintf(stderr,
          ",  Accept one byte of input, storing its value in the byte at the "
          "data \n   pointer.\n");
  fprintf(stderr,
          "[  If the byte at the data pointer is zero, then instead of moving "
          "the \n   instruction pointer forward to the next command, jump it "
          "forward to \n   the command after the matching ] command.\n");
  fprintf(
      stderr,
      "]  If the byte at the data pointer is nonzero, then instead of "
      "moving \n   the instruction pointer forward to the next command, jump "
      "it back to \n   the command after the matching [ command.\n");

  fprintf(stderr, "\n");
  fprintf(stderr, "Any other characters are seen as comments.\n");
}

void console_print_copyright() {
  fprintf(stderr, "Copyright (C) 2023 CS100, Shanghaitech University.\n");
  fprintf(stderr, "All rights reserved.\n");
}

void console_print_credits() {
  fprintf(stderr,
          "Thanks to Linshu Yang, Yuqi Liu, Ke Gong and other teaching "
          "crews for \nsupporting IBF development. See cs100.geekpie.club for "
          "more information.\n");
}

void console_print_license() {
  fprintf(
      stderr,
      "IBF is licensed under the GNU General Public License v3.0. See \n"
      "https://www.gnu.org/licenses/gpl-3.0.en.html for more information.\n");
}

/**
 * @brief Run IBF interactively in the console.
 */
void run_console() {
  fprintf(stderr, "IBF %d.%d.%d (tags/v%d.%d.%d, %s, %s) [%s %s] on %s\n",
          IBF_VERSION_MAJOR, IBF_VERSION_MINOR, IBF_VERSION_PATCH,
          IBF_VERSION_MAJOR, IBF_VERSION_MINOR, IBF_VERSION_PATCH, __DATE__,
          __TIME__, COMPILER_NAME, COMPILER_VERSION, OS_NAME);
  fprintf(stderr,
          "Type \"help\", \"copyright\", \"credits\" or \"license\" for more "
          "information.\n");
  struct brainfuck_context *context = brainfuck_context_new(
      brainfuck_input_handler_stdin, brainfuck_output_handler_stdout);
  char *line = calloc(BRAINFUCK_MAX_LINE_LENGTH + 1, sizeof(char));
  fprintf(stderr, ">>> ");
  while (true) {
    fflush(stdout);
    if (!brainfuck_readline_util(stdin, line, BRAINFUCK_MAX_LINE_LENGTH + 1,
                                 '\n')) {
      fprintf(stderr, ">>> ");
      continue;
    }
    if (strcmp(line, "help") == 0) {
      console_print_help();
    } else if (strcmp(line, "copyright") == 0) {
      console_print_copyright();
    } else if (strcmp(line, "credits") == 0) {
      console_print_credits();
    } else if (strcmp(line, "license") == 0) {
      console_print_license();
    } else {
      brainfuck_main(context, line);
    }
    stdin_flush();
    fprintf(stderr, ">>> ");
  }
  free(line);
  brainfuck_context_free(context);
}

/**
 * @brief Run IBF from a file.
 * @param file The file to run.
 * @return True if the file is run successfully.
 */
bool run_file(FILE *file) {
  struct brainfuck_context *context = brainfuck_context_new(
      brainfuck_input_handler_stdin, brainfuck_output_handler_stdout);
  char *line = calloc(BRAINFUCK_MAX_LINE_LENGTH + 1, sizeof(char));
  while (true) {
    if (feof(file) || !brainfuck_readline_util(
                          file, line, BRAINFUCK_MAX_LINE_LENGTH + 1, '\n')) {
      break;
    }
    if (!brainfuck_main(context, line)) {
      return false;
    }
  }
  free(line);
  if (context->state->loop_size > 0) {
    brainfuck_context_free(context);
    print_error_unmatched_loop_start();
    return false;
  }
  brainfuck_context_free(context);
  return true;
}

/**
 * @brief Run IBF from a command.
 * @param command The command to run.
 * @return True if the command is run successfully.
 */
bool run_command(char *command) {
  int32_t unmatched_loop = 0;
  for (size_t i = 0; command[i] != '\0'; i++) {
    switch (command[i]) {
      case '[':
        unmatched_loop += 1;
        break;
      case ']':
        unmatched_loop -= 1;
        break;
      default:
        break;
    }
  }
  if (unmatched_loop < 0) {
    print_error_unmatched_loop_end();
    return false;
  } else if (unmatched_loop > 0) {
    print_error_unmatched_loop_start();
    return false;
  }
  struct brainfuck_context *context = brainfuck_context_new(
      brainfuck_input_handler_stdin, brainfuck_output_handler_stdout);
  if (!brainfuck_main(context, command)) {
    brainfuck_context_free(context);
    return false;
  }
  brainfuck_context_free(context);
  return true;
}

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
  fprintf(stderr, "usage: ibf [options] ... [-c cmd | file]\n");
  fprintf(stderr, "Try `ibf -h` for more information.\n");
}

/**
 * @brief Print the help of IBF.
 */
void print_help() {
  fprintf(stderr, "usage: ibf [options] ... [-c cmd | file]\n");
  fprintf(stderr, "Options and arguments:\n");
  fprintf(stderr, "-v, --version\t  : Print the version of IBF.\n");
  fprintf(stderr, "-h, --help\t  : Print the help of IBF.\n");
  fprintf(stderr, "-c, --cmd\t  : Run program passed in as string. \n");
  fprintf(stderr, "file\t\t  : Program read from script file.\n");
}

/**
 * @brief Options for IBF.
 * Version: -v, --version. Print the version of the program.
 * Help: -h, --help. Print the help of the program.
 * Command: -c, --command. Run the command from the command line.
 */
static struct option long_options[] = {{"version", no_argument, 0, 'v'},
                                       {"help", no_argument, 0, 'h'},
                                       {"cmd", required_argument, 0, 'c'},
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
        if (!run_command(optarg)) {
          return EXIT_FAILURE;
        }
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
    if (!run_file(file)) {
      fclose(file);
      return EXIT_FAILURE;
    }
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
