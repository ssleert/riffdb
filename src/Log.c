#include "Log.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

LOG_VERBOSITY LogMaxVerbosity = LOG_VERBOSITY_Trace;
bool LogColored = true;
bool LogAddNewLine = true;
bool LogAddDate = false;

static const char* LogVerbosityStrings[] = {
  "why you're watching inside binary?",
  "[TRACE]",
  "[INFO] ",
  "[WARN] ",
  "[ERROR]",
  "[FATAL]",
};

static const char* LogVerbosityStringsColored[] = {
  "sfome on the swag",          "[\033[0;34mTRACE\033[1;0m]",
  "[\033[0;32mINFO\033[1;0m] ", "[\033[0;33mWARN\033[1;0m] ",
  "[\033[0;31mERROR\033[1;0m]", "[\033[0;31mFATAL\033[1;0m]",
};

#ifdef _WIN32

static inline int
LogCurrentTime(struct tm* result)
{
  time_t t = time(NULL);
  return localtime_s(result, &t);
}

#else

static inline int
LogCurrentTime(struct tm* result)
{
  time_t t = time(NULL);
  return localtime_r(&t, result) == NULL;
}

#endif

void
LogFlog(LOG_VERBOSITY Verbosity,
        FILE* Stream,
        size_t Line,
        const char Filename[],
        const char Fmt[],
        ...)
{
  assert(Verbosity >= 0 && Verbosity <= (LOG_VERBOSITY)LOG_VERBOSITY_LEN);
  assert(Stream != NULL);
  assert(Filename != NULL);
  assert(Fmt != NULL);
  assert(strlen(Fmt) < LOG_H_BUFSIZE);

  if (LogMaxVerbosity == LOG_VERBOSITY_None ||
      Verbosity == LOG_VERBOSITY_None) {
    return;
  }
  if (Verbosity < LogMaxVerbosity) {
    return;
  }

  char TimeBuffer[64];
  char StringBuffer[LOG_H_BUFSIZE + 256];
  char* StringPointer = StringBuffer;
  bool LocalLogColored =
    ((Stream == stdout || Stream == stderr) ? (int)LogColored : false) != 0;

  struct tm tm_buf;
  LogCurrentTime(&tm_buf);

  strftime(TimeBuffer,
           sizeof(TimeBuffer),
           (int)LogAddDate ? "%Y-%m-%d %H:%M:%S" : "%H:%M:%S",
           &tm_buf);

  StringPointer +=
    sprintf(StringPointer,
            "%s %s %s:%zu: ",
            TimeBuffer,
            (int)LocalLogColored ? LogVerbosityStringsColored[Verbosity]
                                 : LogVerbosityStrings[Verbosity],
            Filename,
            Line);

  va_list Args;
  va_start(Args, Fmt);
  size_t w = vsnprintf(StringPointer, LOG_H_BUFSIZE - 1, Fmt, Args);
  va_end(Args);

  StringPointer += (w >= LOG_H_BUFSIZE) ? LOG_H_BUFSIZE - 1 : w;

  if (LogAddNewLine) {
    StringPointer[0] = '\n';
    StringPointer[1] = '\0';
  }

  fputs(StringBuffer, Stream);
}
