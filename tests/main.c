#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main()
{
  printf ("Hello world I'm %d\n", getpid ());
  while (1)
    sleep (3600);
}
