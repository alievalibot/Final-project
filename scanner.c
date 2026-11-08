#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "scanner.h"

Token tokens[10000];
static char *input;
static const char *current;
static int line;

void init_scanner(const char *file_name) {
  FILE *file = fopen(file_name, "rb");
  long length;
  if (!file) { perror(file_name); exit(EXIT_FAILURE); }
  if (fseek(file, 0, SEEK_END) || (length = ftell(file)) < 0 ||
      fseek(file, 0, SEEK_SET)) {
    fprintf(stderr, "Cannot read %s\n", file_name);
    fclose(file);
    exit(EXIT_FAILURE);
  }
  free(input);
  input = malloc((size_t)length + 1);
  if (!input) { fclose(file); exit(EXIT_FAILURE); }
  if (fread(input, 1, (size_t)length, file) != (size_t)length) {
    fprintf(stderr, "Cannot read %s\n", file_name);
    fclose(file);
    exit(EXIT_FAILURE);
  }
  fclose(file);
  input[length] = '\0';
  current = input;
  line = 1;
}

static int digit(char c) { return c >= '0' && c <= '9'; }
static int alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static Token scan_token(void) {
  Token token;
  for (;;) {
    if (*current == ';') {
      while (*current && *current != '\r' && *current != '\n') current++;
    } else if (*current == '\r' || *current == '\n') {
      char newline = *current++;
      if (newline == '\r' && *current == '\n') current++;
      line++;
    } else if (*current == ' ' || *current == '\t') current++;
    else break;
  }
  token.source = current;
  token.line = line;
  token.value = 0;
  token.type = TOKEN_ERROR;
  if (!*current) token.type = TOKEN_EOF;
  else if (*current == ',') { current++; token.type = TOKEN_COMMA; }
  else if (digit(*current)) {
    int overflow = 0;
    token.type = TOKEN_INTEGER;
    while (digit(*current)) {
      int n = *current++ - '0';
      if (token.value > (INT_MAX - n) / 10) overflow = 1;
      else if (!overflow) token.value = token.value * 10 + n;
    }
    if (overflow) token.type = TOKEN_ERROR;
  } else if (alpha(*current)) {
    static const struct { const char *name; TokenType type; } keywords[] = {
      {"mov", TOKEN_MOV}, {"or", TOKEN_OR}, {"add", TOKEN_ADD},
      {"sub", TOKEN_SUB}, {"nand", TOKEN_NAND}, {"and", TOKEN_AND},
      {"xor", TOKEN_XOR}, {"eq", TOKEN_EQ}
    };
    size_t i, length;
    while (alpha(*current) || digit(*current)) current++;
    length = (size_t)(current - token.source);
    if (length == 4 && !memcmp(token.source, "reg", 3) &&
        token.source[3] >= '1' && token.source[3] <= '6') {
      token.type = TOKEN_REGISTER;
      token.value = token.source[3] - '0';
    } else {
      for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
        if (strlen(keywords[i].name) == length &&
            !memcmp(token.source, keywords[i].name, length)) {
          token.type = keywords[i].type;
          break;
        }
      }
    }
  } else current++;
  token.length = (int)(current - token.source);
  return token;
}

void scan_tokens(void) {
  size_t i;
  for (i = 0; i < sizeof(tokens) / sizeof(tokens[0]); i++) {
    tokens[i] = scan_token();
    if (tokens[i].type == TOKEN_EOF) return;
  }
  fprintf(stderr, "Error: source exceeds token capacity.\n");
  exit(EXIT_FAILURE);
}
