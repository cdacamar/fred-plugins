#ifndef LEX_HELPERS_H
#define LEX_HELPERS_H

// Lexing helpers.
static uint32_t lex_is_space(char c) {
  return c == ' ' || c == '\t';
}

static uint32_t lex_is_lf(char c) {
  return c == '\n';
}

static uint32_t lex_is_lower(char c) {
  return c >= 'a' && c <= 'z';
}

static uint32_t lex_is_upper(char c) {
  return c >= 'A' && c <= 'Z';
}

static uint32_t lex_is_alpha(char c) {
  return lex_is_lower(c) || lex_is_upper(c);
}

static uint32_t lex_is_digit(char c) {
  return c >= '0' && c <= '9';
}

static uint32_t lex_is_xdigit(char c) {
  return lex_is_digit(c) || ('a' <= c && c <= 'f') || ('A' <= c && c <= 'F');
}

static uint32_t lex_is_binary_digit(char c) {
  return c == '0' || c == '1';
}

static uint32_t lex_is_octal_digit(char c) {
  return '0' <= c && c <= '7';
}

static uint32_t lex_is_key_first(char c) {
  return c == '-' || c == '_' || lex_is_alpha(c) || lex_is_digit(c);
}

static uint32_t lex_is_string_match(String8 a, String8 b) {
  if (a.size != b.size)
    return 0;
  return memcmp(a.str, b.str, a.size) == 0;
}

typedef uint32_t(*PredFn)(char);

static uint64_t lex_skip_pred(String8 str, uint64_t off, PredFn pred) {
  for (;off < str.size; ++off) {
    if (!pred(str.str[off]))
      break;
  }
  return off;
}

#endif // LEX_HELPERS_H