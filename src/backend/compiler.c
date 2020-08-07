#include <stdio.h>
#include <stdlib.h>

#include "compiler.h"
#include "scanner.h"
#include "chunk.h"

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

typedef void (*ParseFunction)(Parser* parser);

typedef struct
{
    ParseFunction prefix;
    ParseFunction infix;
    Precedence precedence;
} ParseRule;

static void expression();
static ParseRule *get_rule(const TokenType type);
static void parse_precedence(Parser *parser, const Precedence precedence);

static void error_at(Parser *parser, const Token *token, const char *message)
{
    if (parser->panic_mode)
    {
        return;
    }

    parser->panic_mode = true;

    fprintf(stderr, "[line %zu] Error", token->line);

    if (token->type == TOKEN_EOF)
    {
        fprintf(stderr, " at end");
    }
    else if (token->type == TOKEN_ERROR)
    {
    }
    else
    {
        fprintf(stderr, " at %s", message);
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

Parser new_parser(const char *source_code)
{
    Parser parser;

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

static void consume(Parser *parser, const TokenType type, const char *message)
{
    if (parser->current.type == type)
    {
        advance(parser);
        return;
    }

    error_at_current(parser, message);
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
        error(parser, "Expect expression");
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
    consume(parser, TOKEN_RIGHT_PAREN, "Expect ')' after expression");
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
    default:
        return;
    }
}

static void number(Parser *parser)
{
    const double value = strtod(parser->previous.start, NULL);
    emit_constant(parser, value);
}

static void binary(Parser *parser)
{
    const TokenType operator_type = parser->previous.type;

    ParseRule *rule = get_rule(operator_type);

    parse_precedence(parser, (Precedence)(rule->precedence + 1));

    switch (operator_type)
    {
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

ParseRule rules[] ={
    [TOKEN_LEFT_PAREN] ={ grouping, NULL, PREC_NONE },
    [TOKEN_RIGHT_PAREN] ={ NULL, NULL, PREC_NONE },
    [TOKEN_LEFT_BRACE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_RIGHT_BRACE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_COMMA] ={ NULL, NULL, PREC_NONE },
    [TOKEN_DOT] ={ NULL, NULL, PREC_NONE },
    [TOKEN_MINUS] ={ unary, binary, PREC_TERM },
    [TOKEN_PLUS] ={ NULL, binary, PREC_TERM },
    [TOKEN_SEMICOLON] ={ NULL, NULL, PREC_NONE },
    [TOKEN_SLASH] ={ NULL, binary, PREC_FACTOR },
    [TOKEN_STAR] ={ NULL, binary, PREC_FACTOR },
    [TOKEN_BANG] ={ NULL, NULL, PREC_NONE },
    [TOKEN_BANG_EQUAL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_EQUAL_EQUAL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_GREATER] ={ NULL, NULL, PREC_NONE },
    [TOKEN_GREATER_EQUAL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_LESS] ={ NULL, NULL, PREC_NONE },
    [TOKEN_LESS_EQUAL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_IDENTIFIER] ={ NULL, NULL, PREC_NONE },
    [TOKEN_STRING] ={ NULL, NULL, PREC_NONE },
    [TOKEN_NUMBER] ={ number, NULL, PREC_NONE },
    [TOKEN_AND] ={ NULL, NULL, PREC_NONE },
    [TOKEN_CLASS] ={ NULL, NULL, PREC_NONE },
    [TOKEN_ELSE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_FALSE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_FOR] ={ NULL, NULL, PREC_NONE },
    [TOKEN_FUN] ={ NULL, NULL, PREC_NONE },
    [TOKEN_IF] ={ NULL, NULL, PREC_NONE },
    [TOKEN_NIL] ={ NULL, NULL, PREC_NONE },
    [TOKEN_OR] ={ NULL, NULL, PREC_NONE },
    [TOKEN_PRINT] ={ NULL, NULL, PREC_NONE },
    [TOKEN_RETURN] ={ NULL, NULL, PREC_NONE },
    [TOKEN_SUPER] ={ NULL, NULL, PREC_NONE },
    [TOKEN_THIS] ={ NULL, NULL, PREC_NONE },
    [TOKEN_TRUE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_VAR] ={ NULL, NULL, PREC_NONE },
    [TOKEN_WHILE] ={ NULL, NULL, PREC_NONE },
    [TOKEN_ERROR] ={ NULL, NULL, PREC_NONE },
    [TOKEN_EOF] ={ NULL, NULL, PREC_NONE },
};

static ParseRule *get_rule(const TokenType type)
{
    return &rules[type];
}

bool compile(const char *source_code, Chunk *chunk)
{
    Parser parser = new_parser(source_code);

    compiling_chunk = chunk;

    advance(&parser);
    expression(&parser);
    consume(&parser, TOKEN_EOF, "Expected end of expression");

    end_compiler(&parser);

    return !parser.had_error;
}