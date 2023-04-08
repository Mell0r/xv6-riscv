#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main()
{
  const int SZ = 10000;
  char buff[SZ];
  int res = dmesg(buff);
  buff[SZ - 1] = 0;
  for (int i = 0; i < res; i++) {
    if (buff[i] != '\0')
      write(1, buff + i, 1);
    else
      write(1, "\n", 1);
  }
  exit(0);
}
