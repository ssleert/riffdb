#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t Port;
    char *Directory;
    uint8_t Threads;
    bool ShowHelp;
    bool ShowVersion;
} Options;

extern Options GOptions;

int ParseOptions(int32_t Argc, char *Argv[]);
void FreeOptions(void);
void PrintUsage(const char *ProgramName);
void PrintVersion(const char *ProgramName, const char *Version);

#endif
