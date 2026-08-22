#include "Options.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEFAULT_PORT 9889

Options GOptions;

static char*
strdup(const char* str)
{
  size_t siz;
  char* copy;

  siz = strlen(str) + 1;
  if ((copy = malloc(siz)) == NULL)
    return (NULL);
  (void)memcpy(copy, str, siz);
  return (copy);
}

static uint16_t
GetDefaultThreads(void)
{
  uint16_t N = (uint16_t)sysconf(_SC_NPROCESSORS_ONLN);
  return N;
}

void
PrintUsage(const char* ProgramName)
{
  fprintf(stdout,
          "Usage: %s [OPTIONS]\n"
          "\n"
          "Options:\n"
          "  -p, --port PORT          Listen port (default: %d)\n"
          "  -d, --directory DIR      Working directory (default: current "
          "directory)\n"
          "  -t, --threads N          Number of threads (default: number of "
          "logical CPU cores)\n"
          "  -h, --help               Show this help message and exit\n"
          "  -v, --version            Show version information and exit\n"
          "\n",
          ProgramName,
          DEFAULT_PORT);
}

void
PrintVersion(const char* ProgramName, const char* Version)
{
  printf("%s %s\n", ProgramName, Version);
}

void
FreeOptions(void)
{
  free(GOptions.Directory);
  GOptions.Directory = NULL;
}

int
ParseOptions(int32_t Argc, char* Argv[])
{
  free(GOptions.Directory);

  GOptions.Port = DEFAULT_PORT;
  GOptions.Directory = strdup(".");
  GOptions.Threads = GetDefaultThreads();
  GOptions.ShowHelp = 0;
  GOptions.ShowVersion = 0;

  if (GOptions.Directory == NULL)
    return -1;

  static const struct option LongOptions[] = {
    { "port", required_argument, NULL, 'p' },
    { "directory", required_argument, NULL, 'd' },
    { "threads", required_argument, NULL, 't' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'v' },
    { NULL, 0, NULL, 0 }
  };

  const char* ShortOptions = "p:d:t:hv";
  int32_t Opt;
  int32_t OptionIndex = 0;

  while ((Opt = getopt_long(
            Argc, Argv, ShortOptions, LongOptions, &OptionIndex)) != -1) {
    switch (Opt) {
      case 'p': {
        char* End = NULL;
        long Val = strtol(optarg, &End, 10);
        if (End == optarg || *End != '\0' || Val < 1 || Val > 65535) {
          fprintf(stderr, "%s: invalid port number '%s'\n", Argv[0], optarg);
          return -1;
        }
        GOptions.Port = (uint16_t)Val;
        break;
      }
      case 'd': {
        char* Copy = strdup(optarg);
        if (Copy == NULL)
          return -1;
        free(GOptions.Directory);
        GOptions.Directory = Copy;
        break;
      }
      case 't': {
        char* End = NULL;
        long Val = strtol(optarg, &End, 10);
        if (End == optarg || *End != '\0' || Val < 1 || Val > 255) {
          fprintf(stderr, "%s: invalid thread count '%s'\n", Argv[0], optarg);
          return -1;
        }
        GOptions.Threads = (uint8_t)Val;
        break;
      }
      case 'h':
        GOptions.ShowHelp = true;
        break;
      case 'v':
        GOptions.ShowVersion = true;
        break;
      case '?':
        return -1;
      default:
        return -1;
    }
  }

  if (optind < Argc) {
    fprintf(stderr, "%s: unexpected argument(s):", Argv[0]);
    while (optind < Argc)
      fprintf(stderr, " %s", Argv[optind++]);
    fputc('\n', stderr);
    return -1;
  }

  return 0;
}
