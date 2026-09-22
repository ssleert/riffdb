#include "Greeting.h"

#include "stddef.h"
#include <stdio.h>

void
Greeting(void)
{
  static const char* ascii[] = {
    "                                    ",
    "      .S_sSSs     .S    sSSs    sSSs",
    "     .SS~YS%%b   .SS   d%%SP   d%%SP",
    "     S%S   `S%b  S%S  d%S'    d%S'  ",
    "     S%S    S%S  S%S  S%S     S%S   ",
    "     S%S    d*S  S&S  S&S     S&S   ",
    "     S&S   .S*S  S&S  S&S_Ss  S&S_Ss",
    "     S&S_sdSSS   S&S  S&S~SP  S&S~SP",
    "     S&S~YSY%b   S&S  S&S     S&S   ",
    "     S*S   `S%b  S*S  S*b     S*b   ",
    "     S*S    S%S  S*S  S*S     S*S   ",
    "     S*S    S&S  S*S  S*S     S*S   ",
    "     S*S    SSS  S*S  S*S     S*S   ",
    "     SP          SP   SP      SP    ",
    "     Y           Y    Y       Y     ",
    "                                    ",
    "        straight from the devil     ",
    "                                    ",
  };

  for (size_t i = 0; i < sizeof(ascii) / sizeof(ascii[0]); ++i) {
    puts(ascii[i]);
  }
}
