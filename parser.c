#include <stdio.h>
#include "scanner.h"
#include "parser.h"

static u8 final_binary[255];
static int byte_count;
static int error_count;

static int report(int line, const char *message) {
  fprintf(stderr, "Error on line %d: %s\n", line, message);
  error_count++;
  return 0;
}

static int compile_line(const Token *t, int count) {
  int opcode = -1;
  int value;
  int i;
  for (i = 0; i < count; i++) {
    if (t[i].type == TOKEN_ERROR)
      return report(t[i].line, "Unknown identifier, invalid character, or integer out of range.");
  }
  if (t[0].type == TOKEN_MOV) {
    if (count != 4 || t[1].type != TOKEN_REGISTER ||
        t[2].type != TOKEN_COMMA ||
        (t[3].type != TOKEN_INTEGER && t[3].type != TOKEN_REGISTER))
      return report(t[0].line, "Expected mov REGISTER, REGISTER or mov reg1, CONSTANT.");
    if (t[3].type == TOKEN_INTEGER) {
      if (t[1].value != 1)
        return report(t[0].line, "Can only move a constant into register 1.");
      if (t[3].value > 63)
        return report(t[0].line, "Constant must be between 0 and 63.");
      value = t[3].value;
    } else {
      value = 0x80 | (t[3].value << 3) | t[1].value;
    }
  } else {
    switch (t[0].type) {
      case TOKEN_OR: opcode = 0; break;
      case TOKEN_ADD: opcode = 1; break;
      case TOKEN_SUB: opcode = 2; break;
      case TOKEN_NAND: opcode = 3; break;
      case TOKEN_AND: opcode = 4; break;
      case TOKEN_XOR: opcode = 6; break;
      case TOKEN_EQ: opcode = 7; break;
      default: return report(t[0].line, "Expected an instruction.");
    }
    if (count != 6 || t[1].type != TOKEN_REGISTER ||
        t[2].type != TOKEN_COMMA || t[3].type != TOKEN_REGISTER ||
        t[4].type != TOKEN_COMMA || t[5].type != TOKEN_REGISTER)
      return report(t[0].line, "Expected instruction REGISTER, reg1, reg2.");
    if (t[3].value != 1 || t[5].value != 2)
      return report(t[0].line, "Source operands must be reg1 followed by reg2.");
    value = 0x40 | (t[1].value << 3) | opcode;
  }
  if (byte_count == (int)sizeof(final_binary))
    return report(t[0].line, "Program exceeds the 255-byte output capacity.");
  final_binary[byte_count++] = (u8)value;
  return 1;
}

int parse_lines(void) {
  const Token *current = tokens;
  byte_count = 0;
  error_count = 0;
  while (current->type != TOKEN_EOF) {
    const Token *start = current;
    int line = current->line;
    while (current->type != TOKEN_EOF && current->line == line) current++;
    compile_line(start, (int)(current - start));
  }
  return error_count;
}

void print_binary(void) {
  int i, bit;
  if (error_count) return;
  for (i = 0; i < byte_count; i++) {
    for (bit = 7; bit >= 0; bit--)
      putchar((final_binary[i] & (1 << bit)) ? '1' : '0');
    putchar('\n');
  }
}
