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

typedef void (*ParseFunction)(Parser *parser, Precedence precedence);

typedef struct
{
  ParseFunction prefix;
  ParseFunction infix;
  Precedence precedence;
} ParseRule;

// We could use a chain of hash tables to keep track
// of variables declared in each scope, but that's too slow.
//
// To keep track of variables we will number them based on how
// many scopes they are nested in.
typedef struct
{
  Token name;
  // [depth] keeps track of how many scopes are surrounding
  // this local.
  // [depth] will be 0 when the variable is declared
  // in the global scope.
  // TODO: will [depth] ever be 0 since we have another
  // mechanism to declare globals?
  int depth;
} Local;

typedef struct
{
  // List of locals that are in scope during each point
  // during the compilation process.
  // The order of the local in [locals] is the order which
  // the local is declared in the code.
  Local locals[UINT8_COUNT];
  // [local_count] counts how many locals are in scope,
  // in another words, [local_count] is the length of [locals].
  int local_count;
  // [scope_depth] keeps track of the number of blocks
  // that introduce new scopes surrouding the current piece of
  // code that we are compiling.
  int scope_depth;
} Compiler;

Compiler new_compiler()
{
  Compiler compiler;

  compiler.local_count = 0;
  compiler.scope_depth = 0;

  return compiler;
}

static bool identifiers_equal(Token *a, Token *b)
{
  if (a->length != b->length)
  {
    return false;
  }

  return memcmp(a->start, b->start, a->length) == 0;
}

// Looks for a local variable by name.
// If the variable is found, returns its index.
// If the variable is not found, returns -1.
static int resolve_local(Compiler *compiler, Token *name)
{
  for (int i = compiler->local_count - 1; i >= 0; i--)
  {
    Local *local = &compiler->locals[i];

    if (identifiers_equal(name, &local->name))
    {
      return i;
    }
  }

  return -1;
}

// A scope can only contain [UINT8_COUNT] local variable declarations.
static bool reached_maximum_number_of_locals(Compiler *compiler)
{
  return compiler->local_count == UINT8_COUNT;
}

// Adds token that contains the local variable name
// to the list of locals.
static void add_local(Compiler *compiler, Parser *parser, Token name)
{
  if (reached_maximum_number_of_locals(compiler))
  {
    error(parser, "Too many local variable declarations");
    return;
  }
  Local *local = &compiler->locals[compiler->local_count++];
  local->name = name;
  local->depth = compiler->scope_depth;
}

static bool is_compiling_local_scope(Compiler *compiler)
{
  return compiler->scope_depth > 0;
}

static void begin_scope(Compiler *compiler)
{
  compiler->scope_depth++;
}

// Emits opcode to pop local variables that are in the current
// scope from the stack starting from the variables that were
// declared last.
static void clear_locals_in_the_current_scope(Compiler *compiler, Parser *parser)
{
  while (compiler->local_count > 0 &&
         compiler->locals[compiler->local_count - 1].depth == compiler->scope_depth)
  {
    // TODO:
    // A operation that pops n values from the stack
    // could be an optimization instead of emiting [OP_POP]
    // n times.
    emit_byte(parser, OP_POP);
    compiler->local_count--;
  }
}

static void end_scope(Compiler *compiler, Parser *parser)
{
  // When we leave a scope, the variables inside of it should
  // be cleaned because they won't be used anymore.
  //
  // { -- begin_scope
  //    var x = 10
  // } -- end_scope (x can be cleared)
  clear_locals_in_the_current_scope(compiler, parser);

  compiler->scope_depth--;
}

static void expression(Parser *parser);
static void declaration(Compiler *compiler, Parser *parser);
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

  prefix_rule(parser, precedence);

  while (precedence <= get_rule(parser->current.type)->precedence)
  {
    advance(parser);

    ParseFunction infix_rule = get_rule(parser->previous.type)->infix;

    infix_rule(parser, precedence);
  }
}

static void expression(Parser *parser)
{
  parse_precedence(parser, PREC_ASSIGNMENT);
}

static void grouping(Parser *parser, Precedence _)
{
  expression(parser);
  consume(parser, TOKEN_RIGHT_PAREN);
}

static void unary(Parser *parser, Precedence _)
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

static void number(Parser *parser, Precedence _)
{
  const double value = strtod(parser->previous.start, NULL);
  emit_constant(parser, NUMBER_VAL(value));
}

static void string(Parser *parser, Precedence _)
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

static uint8_t identifier_constant(Parser *parser, Token *name)
{
  Value string = OBJ_VAL(copy_string(parser, name->start, name->length));
  // The identifier string is too long to go in the bytecode,
  // so we add it as a constant to the chunk's
  // constants and return its index because the index,
  // which will go in the bytecode.
  //
  // At runtime, we will access the chunk constants
  // using the index that was added to the bytecode.
  return make_constant(parser, string);
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

static void named_variable(Compiler *compiler, Parser *parser, Token name, Precedence precedence)
{
  uint8_t get_op, set_op;

  int arg = resolve_local(compiler, &name);
  if (arg != -1)
  {
    get_op = OP_GET_LOCAL;
    set_op = OP_SET_LOCAL;
  }
  else
  {

    // We add the identifier to the chunk constants
    // and add its index to the bytecode.
    // At runtime we will get the identifier from the chunk
    // constants using the index that's in the bytecode.
    arg = identifier_constant(parser, &name);
    get_op = OP_GET_GLOBAL;
    set_op = OP_SET_GLOBAL;
  }

  // If variable is being used in assigment:
  // α = β
  if (precedence <= PREC_ASSIGNMENT && advance_if_current_token_is(parser, TOKEN_EQUAL))
  {
    // Compile β since α has already been compiled.
    expression(parser);
    emit_bytes(parser, set_op, arg);
  }
  else
  {
    // If variable is not being used in assignment
    emit_bytes(parser, get_op, arg);
  }
}

static void variable(Compiler *compiler, Parser *parser, Precedence precedence)
{
  named_variable(compiler, parser, parser->previous, precedence);
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
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
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

// Differences between expression and expression statement
//
// We can have a list of statements, that are made of
// actual statements:
//
// print 2 + 2; -- print statement
// print 3 + 3; -- print statement
//
// But we could we have a list of statemtents where
// the statements aren't actually statements. They are expressions:
//
// f(); -- call expression which may contain side effects
//
// To support that, we wrap expressions in a statement
// and say we have an expression statement.
static void expression_statement(Parser *parser)
{
  expression(parser);
  consume(parser, TOKEN_SEMICOLON);
  // We are emitting OP_POP because expression statements
  // are always discarded.
  emit_byte(parser, OP_POP);
}

static void declare_variable(Compiler *compiler, Parser *parser)
{
  if (!is_compiling_local_scope(compiler))
  {
    return;
  }

  Token *name = &parser->previous;
  add_local(compiler, parser, *name);
}

static uint8_t parse_variable(Compiler *compiler, Parser *parser)
{
  consume(parser, TOKEN_IDENTIFIER);

  declare_variable(compiler, parser);

  if (is_compiling_local_scope(compiler))
  {
    // Local variables will not be looked up by name at runtime,
    // so theres no need to add the variable's name to the constant table.
    return 0;
  }

  return identifier_constant(parser, &parser->previous);
}

// [global] is the index of the variable in the chunk
// constants list.
//
// At runtime we use [global] to access the actual value
// thats in the chunk constants list.
static void define_variable(Compiler *compiler, Parser *parser, uint8_t global)
{
  if (is_compiling_local_scope(compiler))
  {
    return;
  }

  emit_bytes(parser, OP_DEFINE_GLOBAL, global);
}

// var α = β;
static void var_declaration(Compiler *compiler, Parser *parser)
{
  uint8_t global_variable = parse_variable(compiler, parser);

  consume(parser, TOKEN_EQUAL);

  expression(parser);

  consume(parser, TOKEN_SEMICOLON);

  define_variable(compiler, parser, global_variable);
}

// What is synchronizing?
//
// When an error happens because some part of the code
// is not valid, the compiler will enter panic mode.
//
// Panic mode means we've found an error and haven't recovered
// from it yet.
//
// The point of [synchronize] is to skip tokens until we find
// a token that we consider as a new context, because if an error
// happened the compiler will not be in a valid state, which may cause it
// to reject parts of the code that are actually correct.
//
// We do that because if there may be more errors in the source code,
// and we want to report them all to the user before stopping.
static void synchronize(Parser *parser)
{
  parser->panic_mode = false;

  while (!current_token_is(parser, TOKEN_EOF))
  {
    if (parser->previous.type == TOKEN_SEMICOLON)
    {
      return;
    }

    switch (parser->current.type)
    {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:; /// no-op
    }

    advance(parser);
  }
}

static void block(Compiler *compiler, Parser *parser)
{
  while (!current_token_is(parser, TOKEN_RIGHT_BRACE) && !current_token_is(parser, TOKEN_EOF))
  {
    declaration(compiler, parser);
  }

  consume(parser, TOKEN_RIGHT_BRACE);
}

static void declaration(Compiler *compiler, Parser *parser)
{
  if (advance_if_current_token_is(parser, TOKEN_VAR))
  {
    var_declaration(compiler, parser);
  }
  else if (advance_if_current_token_is(parser, TOKEN_PRINT))
  {
    print_statement(parser);
  }
  else if (advance_if_current_token_is(parser, TOKEN_LEFT_BRACE))
  {
    begin_scope(compiler);
    block(compiler, parser);
    end_scope(compiler, parser);
  }
  else
  {
    expression_statement(parser);
  }

  if (parser->panic_mode)
  {
    synchronize(parser);
  }
}

bool compile(Vm *vm, const char *source_code, Chunk *chunk)
{
  Parser parser = new_parser(vm, source_code);
  Compiler compiler = new_compiler();

  compiling_chunk = chunk;

  advance(&parser);

  while (!current_token_is(&parser, TOKEN_EOF))
  {
    declaration(&compiler, &parser);
  }

  end_compiler(&parser);

  return !parser.had_error;
}