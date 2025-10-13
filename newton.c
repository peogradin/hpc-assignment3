#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

int n_threads = 1;
int n_lines;
int d;

void parse_args(int argc, char *argv[]);

int main(
  int argc,
  char *argv[]
)
{
  parse_args(argc, argv);
  return 0;
}

void parse_args(
  int argc,
  char *argv[]
)
{
  for (int i = 1; i < argc - 1; ++i) {
    const char *arg = argv[i];
    if ( arg[0] == '-' && arg[2] != '\0' ) {
      if ( !isdigit(arg[2]) ) {
        printf("Unknown argument: %s\n", arg);
        continue;
      }
      if ( arg[1] == 't' ) {
        n_threads = atoi(&arg[2]);
        printf("Using %d threads\n", n_threads);
      }
      else if ( arg[1] == 'l' ) {
        n_lines = atoi(&arg[2]);
        printf("Using %d lines\n", n_lines);
      }
    }
  }

  if ( isdigit(argv[argc]) ) {
    d = atoi(argv[argc]);
    printf("Using exponent %d\n", d);
  }
}
