#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Dynamic Array
typedef struct allocator {
  uint8_t *start;
  uint8_t *prev;
  uint8_t *top;
  uint64_t size;
} allocator;

typedef struct dynamic_array {
  allocator *allocator;
  void *items;
  unsigned int item_size;
  unsigned int count;
  unsigned int capacity;
} dynamic_array;

void dynamic_array_init_allocator(dynamic_array *da, unsigned int item_size,
                                  allocator *allocator) {
  da->allocator = allocator;
  da->items = NULL;
  da->item_size = item_size;
  da->count = 0;
  da->capacity = 0;
}

void dynamic_array_init(dynamic_array *da, unsigned int item_size) {
  dynamic_array_init_allocator(da, item_size, NULL);
}

int dynamic_array_get(dynamic_array *da, unsigned int index, void *item) {
  if (!da || !item || index >= da->count || !da->items) {
    return -1;
  }

  void *source = (uint8_t *)da->items + (index * da->item_size);
  memcpy(item, source, da->item_size);
  return 0;
}

int dynamic_array_append(dynamic_array *da, const void *item) {
  if (!da || !item || da->item_size == 0) {
    perror("Invalid dynamic_array passed to append");
    return 1;
  }

  if (da->count >= da->capacity) {
    unsigned int new_capacity = da->capacity * 2;
    if (new_capacity == 0) {
      new_capacity = 4; // default initial capacity
    }

    void *new_items = realloc(da->items, new_capacity * da->item_size);
    if (new_items == NULL) {
      perror("Failed to reallocate dynamic array");
      return 1;
    }

    da->items = new_items;
    da->capacity = new_capacity;
  }

  memcpy((char *)da->items + (da->count * da->item_size), item, da->item_size);
  da->count++;

  return 0;
}

// Tokenization
typedef enum token_kind {
  TOKEN_TERM,

  TOKEN_OPERATION_ADDITION,
  TOKEN_OPERATION_SUBTRACTION,
  TOKEN_OPERATION_MULTIPLICATION,
  TOKEN_OPERATION_DIVISION,

  TOKEN_BRACKET_OPEN,
  TOKEN_BRACKET_CLOSE,

  TOKEN_INVALID,
  TOKEN_END,
} token_kind;

typedef struct token {
  token_kind kind;
  int value;
} token;

void tokenize(dynamic_array *tokens, char *buffer) {
  char *current = buffer;
  token token;

  do {
    if (*current == ' ') {
      current++;
      continue;
    } else if (*current == '\0') {
      token.kind = TOKEN_END;
    } else if (isdigit(*current)) {
      token.kind = TOKEN_TERM;
      char *end;
      token.value = (int)strtol(current, &end, 10);
      current = end;
    } else if (*current == '+') {
      token.kind = TOKEN_OPERATION_ADDITION;
      current++;
    } else if (*current == '-') {
      token.kind = TOKEN_OPERATION_SUBTRACTION;
      current++;
    } else if (*current == '*') {
      token.kind = TOKEN_OPERATION_MULTIPLICATION;
      current++;
    } else if (*current == '/') {
      token.kind = TOKEN_OPERATION_DIVISION;
      current++;
    } else if (*current == '(') {
      token.kind = TOKEN_BRACKET_OPEN;
      current++;
    } else if (*current == ')') {
      token.kind = TOKEN_BRACKET_CLOSE;
      current++;
    } else {
      token.kind = TOKEN_INVALID;
      current++;
    }

    dynamic_array_append(tokens, &token);
  } while (token.kind != TOKEN_END);
}

// Expressions
typedef enum expr_kind {
  EXPR_TERM,

  EXPR_ADD,
  EXPR_SUBTRACT,
  EXPR_MULTIPLY,
  EXPR_DIVIDE,
} expr_kind;

typedef struct expr_node {
  expr_kind kind;
  union {
    struct {
      struct expr_node *left;
      struct expr_node *right;
    } binary;
    int term;
  };
} expr_node;

// Parser
typedef struct parser {
  dynamic_array *tokens;
  int position;
} parser;

void parser_init(dynamic_array *tokens, parser *p) {
  p->tokens = tokens;
  p->position = 0;
}

void parser_current(parser *p, token *token) {
  dynamic_array_get(p->tokens, p->position, token);
}

void parser_advance(parser *p) { p->position++; }

expr_node *parse_expr(parser *p);

expr_node *parse_factor(parser *p) {
  token token;
  parser_current(p, &token);
  if (token.kind == TOKEN_TERM) {
    expr_node *node = malloc(sizeof(expr_node));
    node->kind = EXPR_TERM;
    node->term = token.value;
    parser_advance(p);
    return node;
  } else if (token.kind == TOKEN_BRACKET_OPEN) {
    parser_advance(p);
    expr_node *node = parse_expr(p);
    parser_current(p, &token);
    if (token.kind != TOKEN_BRACKET_CLOSE) {
      printf("Syntax error: expected ')'\n");
      exit(1);
    }
    parser_advance(p);
    return node;
  } else {
    printf("Syntax error: expected term or '('\n");
    exit(1);
  }
}

expr_node *parse_term(parser *p) {
  expr_node *left = parse_factor(p);
  while (1) {
    token token;
    parser_current(p, &token);

    if (token.kind == TOKEN_OPERATION_MULTIPLICATION ||
        token.kind == TOKEN_OPERATION_DIVISION) {
      parser_advance(p);
      expr_node *right = parse_factor(p);

      expr_node *parent = malloc(sizeof(expr_node));
      parent->kind = (token.kind == TOKEN_OPERATION_MULTIPLICATION)
                         ? EXPR_MULTIPLY
                         : EXPR_DIVIDE;
      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }
  return left;
}

expr_node *parse_expr(parser *p) {
  expr_node *left = parse_term(p);
  while (1) {
    token token;
    parser_current(p, &token);

    if (token.kind == TOKEN_OPERATION_ADDITION ||
        token.kind == TOKEN_OPERATION_SUBTRACTION) {
      parser_advance(p);
      expr_node *right = parse_term(p);

      expr_node *parent = malloc(sizeof(expr_node));
      parent->kind =
          (token.kind == TOKEN_OPERATION_ADDITION) ? EXPR_ADD : EXPR_SUBTRACT;
      parent->binary.left = left;
      parent->binary.right = right;
      left = parent;
    } else {
      break;
    }
  }
  return left;
}

float evaluate_expr(expr_node *parent_expr) {
  switch (parent_expr->kind) {
  case EXPR_TERM:
    return (float)parent_expr->term;
  case EXPR_ADD:
    return (float)(evaluate_expr(parent_expr->binary.left) +
                   evaluate_expr(parent_expr->binary.right));
  case EXPR_SUBTRACT:
    return (float)(evaluate_expr(parent_expr->binary.left) -
                   evaluate_expr(parent_expr->binary.right));
  case EXPR_MULTIPLY:
    return (float)(evaluate_expr(parent_expr->binary.left) *
                   evaluate_expr(parent_expr->binary.right));
  case EXPR_DIVIDE:
    return (float)(evaluate_expr(parent_expr->binary.left) /
                   evaluate_expr(parent_expr->binary.right));
  }
}

void free_expr(expr_node *expr) {
  if (!expr)
    return;
  if (expr->kind != EXPR_TERM) {
    free_expr(expr->binary.left);
    free_expr(expr->binary.right);
  }
  free(expr);
}

int main() {
  while (1) {
    char buffer[1024];

    printf("\n>>> ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    dynamic_array tokens;
    dynamic_array_init(&tokens, sizeof(token));
    tokenize(&tokens, buffer);

    parser p;
    parser_init(&tokens, &p);
    expr_node *root_expr = parse_expr(&p);

    printf("  = %.2f", evaluate_expr(root_expr));
    free_expr(root_expr);
  }
}
