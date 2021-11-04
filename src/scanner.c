#include <stdio.h>
#include <string.h>

#include "scanner.h"
#include "common.h"

Scanner new_scanner(const char *source_code)
{
  Scanner scanner;
  scanner.start = source_code;
  scanner.current = source_code;
  scanner.line = 1;
  return scanner;
}

static bool is_at_end_of_source_code(const Scanner *scanner)
{
  return *scanner->current == '\0';
}

static Token new_token(const Scanner *scanner, const TokenType type)
{
  Token token;

  token.type = type;
  token.start = scanner->start;
  token.length = scanner->current - scanner->start;
  token.line = scanner->line;

  return token;
}

static Token new_error_token(const Scanner *scanner, const char *message)
{
  Token token;

  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = strlen(message);
  token.line = scanner->line;

  return token;
}

static char advance(Scanner *scanner)
{
  scanner->current += 1;
  return scanner->current[-1];
}

static bool match(Scanner *scanner, const char expected)
{
  if (is_at_end_of_source_code(scanner))
  {
    return false;
  }

  if (*scanner->current != expected)
  {
    return false;
  }

  scanner->current += 1;

  return true;
}

static char peek(const Scanner *scanner)
{
  return *scanner->current;
}

static char peek_n_ahead(const Scanner *scanner, const size_t offset)
{
  if (is_at_end_of_source_code(scanner))
  {
    return '\0';
  }

  return scanner->current[offset];
}

static void skip_whitespace(Scanner *scanner)
{
  for (;;)
  {
    const char c = peek(scanner);

    switch (c)
    {
    case ' ':
    case '\r':
    case '\t':
      advance(scanner);
      break;
    case '\n':
      scanner->line += 1;
      advance(scanner);
      break;
    case '/':
      if (peek_n_ahead(scanner, 1) == '/')
      {
        while (peek(scanner) != '\n' && is_at_end_of_source_code(scanner))
        {
          advance(scanner);
        }
      }
      else
      {
        return;
      }
      break;
    default:
      return;
    }
  }
}

static Token new_string(Scanner *scanner)
{
  while (peek(scanner) != '"' && is_at_end_of_source_code(scanner))
  {
    if (peek(scanner) == '\n')
    {
      scanner->line += 1;
      advance(scanner);
    }
  }

  if (is_at_end_of_source_code(scanner))
  {
    return new_error_token(scanner, "Unterminated string");
  }

  advance(scanner);

  return new_token(scanner, TOKEN_STRING);
}

static bool is_digit(const char c)
{
  return c >= '0' && c <= '9';
}

static Token new_number(Scanner *scanner)
{
  while (is_digit(peek(scanner)))
  {
    advance(scanner);
  }

  if (peek(scanner) == '.' && is_digit(peek_n_ahead(scanner, 1)))
  {
    advance(scanner);

    while (is_digit(peek(scanner)))
    {
      advance(scanner);
    }
  }

  return new_token(scanner, TOKEN_NUMBER);
}

static bool is_alpha(const char c)
{
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         c == '_' || c == '?';
}

static TokenType check_keyword(const Scanner *scanner, size_t start, size_t length, const char *rest, const TokenType type)
{
  if (scanner->current - scanner->start == start + length &&
      memcmp(scanner->start + start, rest, length) == 0)
  {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(const Scanner *scanner)
{
  switch (scanner->start[0])
  {
  case 'a':
    return check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
  case 'c':
    return check_keyword(scanner, 1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if (scanner->current - scanner->start > 1)
    {
      switch (scanner->start[1])
      {
      case 'a':
        return check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
      case 'u':
        return check_keyword(scanner, 2, 1, "n", TOKEN_FUN);
      }
    }
    break;
  case 'i':
    return check_keyword(scanner, 1, 1, "f", TOKEN_IF);
  case 'n':
    return check_keyword(scanner, 1, 2, "il", TOKEN_NIL);
  case 'o':
    return check_keyword(scanner, 1, 1, "r", TOKEN_OR);
  case 'p':
    return check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return check_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
  case 't':
    if (scanner->current - scanner->start > 1)
    {
      switch (scanner->start[1])
      {
      case 'h':
        return check_keyword(scanner, 2, 2, "is", TOKEN_THIS);
      case 'r':
        return check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v':
    return check_keyword(scanner, 1, 2, "ar", TOKEN_VAR);
  case 'w':
    return check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
  }
  return TOKEN_IDENTIFIER;
}

static Token new_identifier(Scanner *scanner)
{
  while (is_alpha(peek(scanner)) || is_digit(peek(scanner)))
  {
    advance(scanner);
  }

  return new_token(scanner, identifier_type(scanner));
}

Token scan_token(Scanner *scanner)
{
  skip_whitespace(scanner);

  scanner->start = scanner->current;

  if (is_at_end_of_source_code(scanner))
  {
    return new_token(scanner, TOKEN_EOF);
  }

  const char c = advance(scanner);

  if (is_alpha(c))
  {
    return new_identifier(scanner);
  }

  if (is_digit(c))
  {
    return new_number(scanner);
  }

  switch (c)
  {
  case '(':
    return new_token(scanner, TOKEN_LEFT_PAREN);
  case ')':
    return new_token(scanner, TOKEN_RIGHT_PAREN);
  case '{':
    return new_token(scanner, TOKEN_LEFT_BRACE);
  case '}':
    return new_token(scanner, TOKEN_RIGHT_BRACE);
  case ';':
    return new_token(scanner, TOKEN_SEMICOLON);
  case ',':
    return new_token(scanner, TOKEN_COMMA);
  case '.':
    return new_token(scanner, TOKEN_DOT);
  case '-':
    return new_token(scanner, TOKEN_MINUS);
  case '+':
    return new_token(scanner, TOKEN_PLUS);
  case '/':
    return new_token(scanner, TOKEN_SLASH);
  case '*':
    return new_token(scanner, TOKEN_STAR);
  case '!':
    return new_token(scanner, match(scanner, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return new_token(scanner, match(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return new_token(scanner, match(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return new_token(scanner, match(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return new_string(scanner);
  }

  return new_error_token(scanner, "Unexpected character");
}