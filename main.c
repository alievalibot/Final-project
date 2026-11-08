#include <stdio.h>
#include <stdlib.h>
#include "scanner.h"
#include "parser.h"

int main(int argc, char **argv) {
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [assembly-file]\n", argv[0]);
    return EXIT_FAILURE;
  }
  init_scanner(argc == 2 ? argv[1] : "example.asm");
  scan_tokens();
  if (parse_lines()) return EXIT_FAILURE;
  print_binary();
  return EXIT_SUCCESS;
}
