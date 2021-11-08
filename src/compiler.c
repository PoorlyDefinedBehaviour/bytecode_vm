#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "chunk.h"
#include "obj.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

Chunk *compiling_chunk;

typedef enum
{
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} Precedence;

typedef void (*ParseFunction)(Parser *parser);

typedef struct
{
  ParseFunction prefix;
  ParseFunction infix;
  Precedence precedence;
} ParseRule;

static void expression(Parser *parser);
static void statement(Parser *parser);
static void declaration(Parser *parser);
static ParseRule *get_rule(const TokenType type);
static void parse_precedence(Parser *parser, const Precedence precedence);

static void error_at(Parser *parser, const Token *token, const char *message)
{
  if (parser->panic_mode)
  {
    return;
  }

  parser->panic_mode = true;

  fprintf(stderr, "[line %zu] ", token->line);

  if (token->type == TOKEN_EOF)
  {
    fprintf(stderr, " at end\n");
  }
  else if (token->type == TOKEN_ERROR)
  {
  }
  else
  {
    fprintf(stderr, "%s\n", message);
    parser->had_error = true;
  }
}

static void error(Parser *parser, const char *message)
{
  error_at(parser, &parser->previous, message);
}

static void error_at_current(Parser *parser, const char *message)
{
  error_at(parser, &parser->current, message);
}

static void advance(Parser *parser)
{
  parser->previous = parser->current;

  for (;;)
  {
    parser->current = scan_token(&parser->scanner);
    if (parser->current.type != TOKEN_ERROR)
    {
      break;
    }

    error_at_current(parser, parser->current.start);
  }
}

Parser new_parser(Vm *vm, const char *source_code)
{
  Parser parser;

  parser.vm = vm;
  parser.scanner = new_scanner(source_code);
  parser.had_error = false;
  parser.panic_mode = false;

  return parser;
}

static Chunk *get_current_chunk()
{
  return compiling_chunk;
}

static void emit_byte(Parser *parser, const uint8_t byte)
{
  write_chunk(get_current_chunk(), byte, parser->previous.line);
}

static void emit_bytes(Parser *parser, const uint8_t a, const uint8_t b)
{
  emit_byte(parser, a);
  emit_byte(parser, b);
}

static uint8_t make_constant(Parser *parser, const Value value)
{
  const size_t constant = add_constant(get_current_chunk(), value);

  if (constant > UINT8_MAX)
  {
    error(parser, "Too many constants in one chunk");
    return 0;
  }

  return (uint8_t)constant;
}

static void emit_constant(Parser *parser, const Value value)
{
  emit_bytes(parser, OP_CONSTANT, make_constant(parser, value));
}

static void consume(Parser *parser, const TokenType type)
{
  if (parser->current.type == type)
  {
    advance(parser);
    return;
  }

  char buffer[256];

  snprintf(
      buffer,
      256,
      "expected %s, got %s",
      token_type_to_string(type),
      token_type_to_string(parser->current.type));

  error_at_current(parser, buffer);
}

static void emit_return(Parser *parser)
{
  emit_byte(parser, OP_RETURN);
}

static void end_compiler(Parser *parser)
{
  emit_return(parser);

#ifdef DEBUG_PRINT_CODE
  if (!parser->had_error)
  {
    dissasamble_chunk(get_current_chunk(), "code");
  }
#endif
}

static void parse_precedence(Parser *parser, const Precedence precedence)
{
  advance(parser);

  ParseFunction prefix_rule = get_rule(parser->previous.type)->prefix;

  if (prefix_rule == NULL)
  {
    error(parser, "expected expression");
    return;
  }

  prefix_rule(parser);

  while (precedence <= get_rule(parser->current.type)->precedence)
  {
    advance(parser);

    ParseFunction infix_rule = get_rule(parser->previous.type)->infix;

    infix_rule(parser);
  }
}

static void expression(Parser *parser)
{
  parse_precedence(parser, PREC_ASSIGNMENT);
}

static void grouping(Parser *parser)
{
  expression(parser);
  consume(parser, TOKEN_RIGHT_PAREN);
}

static void unary(Parser *parser)
{
  parse_precedence(parser, PREC_UNARY);

  const TokenType operator_type = parser->previous.type;

  expression(parser);

  switch (operator_type)
  {
  case TOKEN_MINUS:
    emit_byte(parser, OP_NEGATE);
    break;
  case TOKEN_BANG:
    emit_byte(parser, OP_NOT);
    break;
  default:
    return;
  }
}

static void number(Parser *parser)
{
  const double value = strtod(parser->previous.start, NULL);
  emit_constant(parser, NUMBER_VAL(value));
}

static void string(Parser *parser)
{
  // Given the following string "hello world":
  // start + 1 removes the first " and
  // previous.length - 2 removes the last ".
  //
  // TODO: this is a constant string, at the moment
  // we are heap allocating it, but it could be put on the stack.
  ObjString *string = copy_string(parser->vm, parser->previous.start + 1, parser->previous.length - 2);

  emit_constant(parser, OBJ_VAL(string));
}

static void binary(Parser *parser)
{
  const TokenType operator_type = parser->previous.type;

  ParseRule *rule = get_rule(operator_type);

  parse_precedence(parser, (Precedence)(rule->precedence + 1));

  switch (operator_type)
  {
  case TOKEN_BANG_EQUAL:
    emit_bytes(parser, OP_EQUAL, OP_NOT);
    break;
  case TOKEN_EQUAL_EQUAL:
    emit_byte(parser, OP_EQUAL);
    break;
  case TOKEN_GREATER:
    emit_byte(parser, OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emit_bytes(parser, OP_LESS, OP_NOT);
    break;
  case TOKEN_LESS:
    emit_byte(parser, OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emit_bytes(parser, OP_GREATER, OP_NOT);
    break;
  case TOKEN_PLUS:
    emit_byte(parser, OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(parser, OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(parser, OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(parser, OP_DIVIDE);
    break;
  default:
    return;
  }
}

static void literal(Parser *parser)
{
  switch (parser->previous.type)
  {
  case TOKEN_FALSE:
    emit_byte(parser, OP_FALSE);
    break;
  case TOKEN_NIL:
    emit_byte(parser, OP_NIL);
    break;
  case TOKEN_TRUE:
    emit_byte(parser, OP_TRUE);
    break;
  default:
    return;
  }
}

ParseRule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

static ParseRule *get_rule(const TokenType type)
{
  return &rules[type];
}

// Returns true when the token that [parser] is currently
// looking at is of type [type].
// Otherwise, returns false.
static bool current_token_is(Parser *parser, TokenType type)
{
  return parser->current.type == type;
}

// If the token [parser] is currently looking at is of [type],
// advances [parser] to next token and returns true.
// Otherwise, returns false.
static bool advance_if_current_token_is(Parser *parser, TokenType type)
{
  if (!current_token_is(parser, type))
  {
    return false;
  }

  advance(parser);

  return true;
}

static void print_statement(Parser *parser)
{
  // Given the following expression:
  //
  // print α;
  //
  // expression will add α to the chunk.
  expression(parser);
  // Consume ; after α
  consume(parser, TOKEN_SEMICOLON);
  // Add OP_PRINT to the chunk.
  //
  // When we are executing the chunk and find an OP_PRINT,
  // we will push it into the stack, after having pushed a value,
  // that will be used by the print operation.
  //
  // ┌────────┐
  // │OP_PRINT│
  // ├────────┤
  // │   α    │
  // ├────────┤
  // │  ...   │
  // ├────────┤
  // │  ...   │
  // └────────┘
  //
  // When we find OP_PRINT in
  emit_byte(parser, OP_PRINT);
}

static void declaration(Parser *parser)
{
  if (advance_if_current_token_is(parser, TOKEN_PRINT))
  {
    print_statement(parser);
  }
}

bool compile(Vm *vm, const char *source_code, Chunk *chunk)
{
  Parser parser = new_parser(vm, source_code);

  compiling_chunk = chunk;

  advance(&parser);

  while (!current_token_is(&parser, TOKEN_EOF))
  {
    declaration(&parser);
  }

  end_compiler(&parser);

  return !parser.had_error;
}