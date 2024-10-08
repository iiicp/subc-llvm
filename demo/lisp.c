// A Lisp interperter with reference counting garbage collection.
// Implemented in pure SysY.
// Written by MaxXing, 2022.

// ======================================================================
// Error handling
// ======================================================================

int printf(const char *fmg, ...);
int scanf(const char *format, ...);
int getchar();
int putchar(int ch);

int getint() {
  int val;
  scanf("%d", &val);
  return val;
}

int getch() { return getchar(); }

int getarray(int val[]) {
  int len;
  for (int i = 0; i < len; i++) {
    scanf("%d", val[i]);
  }
  return len;
}

void putint(int val) { printf("%d", val); }

void putch(int val) { putchar(val); }

void putarray(int len, int arr[]) {
  printf("%d:", len);
  for (int i = 0; i < len; i++) {
    printf(" %d", arr[i]);
  }
}

// Error codes.
const int ERR_INVALID_DATA_TYPE = 0;
const int ERR_BUFFER_OVERFLOW = 1;
const int ERR_PARSE_ERROR = 2;
const int ERR_SYMBOL_NOT_FOUND = 3;
const int ERR_INVALID_LIST = 4;
const int ERR_INVALID_FUNC = 5;
const int ERR_INVALID_ARG_NUM = 6;
const int ERR_TYPE_MISMATCH = 7;
const int ERR_INVALID_SYMBOL = 8;

// Prints error message and panics.
int panic(int code) {
  putch(112);
  putch(97);
  putch(110);
  putch(105);
  putch(99);
  putch(32);
  putint(code);
  putch(10);
  while (1) {
    // do nothing
  }
  return code;
}

// ======================================================================
// Data
// ======================================================================

// Fields of data.
const int DATA_TYPE = 0;
const int DATA_VALUE = 1;
const int DATA_NEXT = 2;
const int DATA_REF_COUNT = 3;
const int DATA_FIELD_COUNT = 4;

// Data.
const int MAX_DATA_LEN = 2048;
int data[2048][2048];
int free_data_ptr = 0;

// Types of data.

// Symbol.
// Value is index of symbol.
const int DATA_TYPE_SYMBOL = 0;
// Number.
// Value is number.
const int DATA_TYPE_NUMBER = 1;
// List.
// Value is the index of the first element.
const int DATA_TYPE_LIST = 2;
// Environment.
// Value is the index of the first symbol-value pair.
// Next is the index of the outer environment.
const int DATA_TYPE_ENV = 3;
// Function.
// Values are parameter list, body, and environment.
const int DATA_TYPE_FUNC = 4;

// Initializes the data list.
void init_data() {
  data[0][DATA_NEXT] = 0;
  int i = 1;
  while (i < MAX_DATA_LEN) {
    data[i][DATA_NEXT] = i - 1;
    i = i + 1;
  }
  free_data_ptr = MAX_DATA_LEN - 1;
}

// Allocates a data element.
// The initial reference count is 1.
int alloc_data() {
  if (!free_data_ptr) panic(ERR_BUFFER_OVERFLOW);
  int i = free_data_ptr;
  free_data_ptr = data[i][DATA_NEXT];
  data[i][DATA_NEXT] = 0;
  data[i][DATA_REF_COUNT] = 1;
  return i;
}

// Decrements the reference count of a data element.
// If the reference count reaches zero, the data element is freed.
void free_data(int data_ptr) {
  if (!data_ptr) return;
  // decrement reference count
  int ref_count = data[data_ptr][DATA_REF_COUNT] - 1;
  data[data_ptr][DATA_REF_COUNT] = ref_count;
  if (ref_count > 0) return;
  // free data element
  if (data[data_ptr][DATA_TYPE] != DATA_TYPE_SYMBOL &&
      data[data_ptr][DATA_TYPE] != DATA_TYPE_NUMBER) {
    free_data(data[data_ptr][DATA_VALUE]);
  }
  free_data(data[data_ptr][DATA_NEXT]);
  data[data_ptr][DATA_NEXT] = free_data_ptr;
  free_data_ptr = data_ptr;
}

// Copies a data pointer.
// Increments the reference count.
int copy_ptr(int data_ptr) {
  if (!data_ptr) return 0;
  data[data_ptr][DATA_REF_COUNT] = data[data_ptr][DATA_REF_COUNT] + 1;
  return data_ptr;
}

// Shallow copies a data element.
// The next pointer will not be copied.
int copy_data(int data_ptr) {
  if (!data_ptr) return 0;
  int new_data_ptr = alloc_data();
  data[new_data_ptr][DATA_TYPE] = data[data_ptr][DATA_TYPE];
  int value_ptr = data[data_ptr][DATA_VALUE];
  if (data[data_ptr][DATA_TYPE] != DATA_TYPE_SYMBOL &&
      data[data_ptr][DATA_TYPE] != DATA_TYPE_NUMBER) {
    value_ptr = copy_ptr(value_ptr);
  }
  data[new_data_ptr][DATA_VALUE] = value_ptr;
  data[new_data_ptr][DATA_NEXT] = 0;
  return new_data_ptr;
}

// Makes a symbol.
int make_symbol(int sym_ptr) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_SYMBOL;
  data[data_ptr][DATA_VALUE] = sym_ptr;
  return data_ptr;
}

// Makes a number.
int make_number(int num) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_NUMBER;
  data[data_ptr][DATA_VALUE] = num;
  return data_ptr;
}

// Makes a list.
int make_list(int head_ptr) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_LIST;
  data[data_ptr][DATA_VALUE] = head_ptr;
  return data_ptr;
}

// ======================================================================
// Symbol
// ======================================================================

// Symbol buffer.
const int MAX_SYM_BUF_LEN = 4096;
int sym_buf[4096];
int next_sym = 0;

// Predefined symbols.
const int SYM_QUOTE = -1;
const int SYM_ATOM = -2;    // atom?
const int SYM_NUMBER = -3;  // number?
const int SYM_EQ = -4;      // eq?
const int SYM_CAR = -5;
const int SYM_CDR = -6;
const int SYM_CONS = -7;
const int SYM_COND = -8;
const int SYM_LAMBDA = -9;
const int SYM_DEFINE = -10;
const int SYM_T = -11;
const int SYM_F = -12;
const int SYM_LIST = -13;
const int SYM_ADD = -14;
const int SYM_SUB = -15;
const int SYM_MUL = -16;
const int SYM_DIV = -17;
const int SYM_GT = -18;
const int SYM_LT = -19;
const int SYM_GE = -20;
const int SYM_LE = -21;
const int SYM_EQ_NUM = -22;  // =
const int PRE_SYM_COUNT = 22;//-SYM_EQ_NUM;

// Table of predefined symbols.
const int PREDEF_SYMS[22][8] = {
    {113, 117, 111, 116, 101},          // quote
    {97, 116, 111, 109, 63},            // atom?
    {110, 117, 109, 98, 101, 114, 63},  // number?
    {101, 113, 63},                     // eq?
    {99, 97, 114},                      // car
    {99, 100, 114},                     // cdr
    {99, 111, 110, 115},                // cons
    {99, 111, 110, 100},                // cond
    {108, 97, 109, 98, 100, 97},        // lambda
    {100, 101, 102, 105, 110, 101},     // define
    {116},                              // t
    {102},                              // f
    {108, 105, 115, 116},               // list
    {43},                               // +
    {45},                               // -
    {42},                               // *
    {47},                               // /
    {62},                               // >
    {60},                               // <
    {62, 61},                           // >=
    {60, 61},                           // <=
    {61}                                // =
};

// Checks if the symbol is a predefined symbol.
// The symbol is given by its index in the symbol buffer, and it must be
// the last symbol in the symbol buffer.
// Returns predefined symbol index if it is, or `sym_ptr` otherwise.
int is_predef_sym(int sym_ptr) {
  int i = 0;
  while (i < PRE_SYM_COUNT) {
    int j = 0, failed = 0;
    while (sym_buf[sym_ptr + j]) {
      if (sym_buf[sym_ptr + j] != PREDEF_SYMS[i][j]) {
        failed = 1;
        break;
      }
      j = j + 1;
    }
    if (!failed && !PREDEF_SYMS[i][j]) {
      next_sym = sym_ptr;
      return -i - 1;
    }
    i = i + 1;
  }
  return sym_ptr;
}

// Checks if two symbols are equal.
int is_eq_sym(int sym1, int sym2) {
  if (sym1 == sym2) return 1;
  if (sym1 < 0 || sym2 < 0) return 0;
  int i = 0;
  while (sym_buf[sym1 + i]) {
    if (sym_buf[sym1 + i] != sym_buf[sym2 + i]) return 0;
    i = i + 1;
  }
  return sym_buf[sym1 + i] == sym_buf[sym2 + i];
}

// Prints the given symbol.
void print_sym(int sym_ptr) {
  if (sym_ptr < 0) {
    int i = 0;
    while (PREDEF_SYMS[-sym_ptr - 1][i]) {
      putch(PREDEF_SYMS[-sym_ptr - 1][i]);
      i = i + 1;
    }
  } else {
    int i = 0;
    while (sym_buf[sym_ptr + i]) {
      putch(sym_buf[sym_ptr + i]);
      i = i + 1;
    }
  }
}

// ======================================================================
// Environment
// ======================================================================

// Makes an environment.
int make_env(int outer_ptr) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_ENV;
  data[data_ptr][DATA_VALUE] = 0;
  data[data_ptr][DATA_NEXT] = outer_ptr;
  return data_ptr;
}

// Adds a symbol-value pair to an environment.
void env_add(int env_ptr, int sym_ptr, int val_data_ptr) {
  // make a new symbol-value pair
  int sym = make_symbol(sym_ptr);
  data[sym][DATA_NEXT] = val_data_ptr;
  int pair = make_list(sym);
  // add it to the environment
  data[pair][DATA_NEXT] = data[env_ptr][DATA_VALUE];
  data[env_ptr][DATA_VALUE] = pair;
}

// Gets the symbol-value pair of a symbol in an environment.
// Returns the pointer if found, or 0 otherwise.
int env_get(int env_ptr, int sym_ptr) {
  int pair = data[env_ptr][DATA_VALUE];
  while (pair) {
    int sym = data[pair][DATA_VALUE];
    if (is_eq_sym(sym_ptr, data[sym][DATA_VALUE])) return pair;
    pair = data[pair][DATA_NEXT];
  }
  return 0;
}

// Sets the value of a symbol in an environment.
// Adds a new symbol-value pair if the symbol is not found.
void env_set(int env_ptr, int sym_ptr, int val_data_ptr) {
  int pair = env_get(env_ptr, sym_ptr);
  if (pair) {
    int sym = data[pair][DATA_VALUE];
    free_data(data[sym][DATA_NEXT]);
    data[sym][DATA_NEXT] = val_data_ptr;
  } else {
    env_add(env_ptr, sym_ptr, val_data_ptr);
  }
}

// Finds the value of a symbol in an environment.
// Returns the value pointer if found, or 0 otherwise.
int env_find(int env_ptr, int sym_ptr) {
  if (!env_ptr) return 0;
  int pair = env_get(env_ptr, sym_ptr);
  if (pair) return data[data[pair][DATA_VALUE]][DATA_NEXT];
  return env_find(data[env_ptr][DATA_NEXT], sym_ptr);
}

// ======================================================================
// Function
// ======================================================================

// Makes a function.
int make_func(int param_list_ptr, int body_ptr, int env_ptr) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_FUNC;
  data[data_ptr][DATA_VALUE] = param_list_ptr;
  data[param_list_ptr][DATA_NEXT] = body_ptr;
  data[body_ptr][DATA_NEXT] = env_ptr;
  return data_ptr;
}

// ======================================================================
// Lexical analysis
// ======================================================================

// Last character read.
int last_char = 32;

// Token type.
const int TOKEN_EOF = 0;
const int TOKEN_SYMBOL = 1;
const int TOKEN_NUMBER = 2;
const int TOKEN_QUOTE = 3;
const int TOKEN_LPAREN = 4;
const int TOKEN_RPAREN = 5;

// Last token read.
// The token is a number or a symbol pointer.
int last_token = 0;

// Checks if the given character is a space.
int is_space(int c) { return c == 32 || c == 9 || c == 10 || c == 13; }

// Checks if the given character is a digit.
int is_digit(int c) { return c >= 48 && c <= 57; }

// Reads a token from the standard input.
// Returns the type of the token.
int next_token() {
  // skip spaces
  while (is_space(last_char)) last_char = getch();
  // handle EOF
  if (last_char == -1) return TOKEN_EOF;
  // handle quote and parentheses
  if (last_char >= 39 && last_char <= 41) {
    int c = last_char;
    last_char = getch();
    return TOKEN_QUOTE + c - 39;
  }
  // handle digits
  if (is_digit(last_char)) {
    int num = 0;
    while (is_digit(last_char)) {
      num = num * 10 + last_char - 48;
      last_char = getch();
    }
    last_token = num;
    return TOKEN_NUMBER;
  }
  // handle symbols
  int sym_ptr = next_sym;
  while (last_char != -1 && !(last_char >= 39 && last_char <= 41) &&
         !is_space(last_char)) {
    sym_buf[next_sym] = last_char;
    next_sym = next_sym + 1;
    last_char = getch();
  }
  sym_buf[next_sym] = 0;
  next_sym = next_sym + 1;
  if (next_sym >= MAX_SYM_BUF_LEN) panic(ERR_BUFFER_OVERFLOW);
  last_token = is_predef_sym(sym_ptr);
  return TOKEN_SYMBOL;
}

// ======================================================================
// Parsing
// ======================================================================

// Last token type.
int last_token_type = 0;//TOKEN_EOF;

// Parses the next data from the token stream.
// Returns the pointer to the data.
// Returns 0 if the token stream is empty.
int parse() {
  if (last_token_type == TOKEN_EOF) return 0;
  if (last_token_type == TOKEN_SYMBOL) {
    int data_ptr = make_symbol(last_token);
    last_token_type = next_token();
    return data_ptr;
  }
  if (last_token_type == TOKEN_NUMBER) {
    int data_ptr = make_number(last_token);
    last_token_type = next_token();
    return data_ptr;
  }
  if (last_token_type == TOKEN_QUOTE) {
    last_token_type = next_token();
    int elem_ptr = parse();
    int quote_ptr = make_symbol(SYM_QUOTE);
    data[quote_ptr][DATA_NEXT] = elem_ptr;
    int list_ptr = make_list(quote_ptr);
    return list_ptr;
  }
  if (last_token_type == TOKEN_LPAREN) {
    last_token_type = next_token();
    int list_ptr = make_list(0), cur_elem = 0;
    while (last_token_type != TOKEN_RPAREN) {
      int elem_ptr = parse();
      if (cur_elem) {
        data[cur_elem][DATA_NEXT] = elem_ptr;
      } else {
        data[list_ptr][DATA_VALUE] = elem_ptr;
      }
      cur_elem = elem_ptr;
    }
    last_token_type = next_token();
    return list_ptr;
  }
  return panic(ERR_PARSE_ERROR);
}

// ======================================================================
// Evaluation
// ======================================================================

// Makes a boolean value.
int make_bool(int value) {
  int data_ptr = alloc_data();
  data[data_ptr][DATA_TYPE] = DATA_TYPE_SYMBOL;
  if (value) {
    data[data_ptr][DATA_VALUE] = SYM_T;
  } else {
    data[data_ptr][DATA_VALUE] = SYM_F;
  }
  return data_ptr;
}

// Checks if the given boolean value is true or null.
int is_true(int bool_ptr) {
  if (!bool_ptr) return 1;
  if (data[bool_ptr][DATA_TYPE] != DATA_TYPE_SYMBOL) panic(ERR_TYPE_MISMATCH);
  if (data[bool_ptr][DATA_VALUE] == SYM_T) return 1;
  if (data[bool_ptr][DATA_VALUE] == SYM_F) return 0;
  return panic(ERR_TYPE_MISMATCH);
}

// Checks if two values are equal.
int is_equal(int val1, int val2) {
  if (val1 == val2) return 1;
  if (!val1 || !val2) return 0;
  if (data[val1][DATA_TYPE] != data[val2][DATA_TYPE]) return 0;
  if (data[val1][DATA_TYPE] == DATA_TYPE_SYMBOL) {
    return is_eq_sym(data[val1][DATA_VALUE], data[val2][DATA_VALUE]);
  }
  if (data[val1][DATA_TYPE] == DATA_TYPE_NUMBER) {
    return data[val1][DATA_VALUE] == data[val2][DATA_VALUE];
  }
  if (data[val1][DATA_TYPE] == DATA_TYPE_LIST) {
    int list1 = data[val1][DATA_VALUE];
    int list2 = data[val2][DATA_VALUE];
    while (list1 && list2) {
      if (!is_equal(list1, list2)) return 0;
      list1 = data[list1][DATA_NEXT];
      list2 = data[list2][DATA_NEXT];
    }
    if (list1 || list2) return 0;
    return 1;
  }
  return panic(ERR_TYPE_MISMATCH);
}

// Unwraps the given list.
// Returns the first element of the list.
int unwrap_list(int list_ptr) {
  if (data[list_ptr][DATA_TYPE] != DATA_TYPE_LIST) panic(ERR_TYPE_MISMATCH);
  return data[list_ptr][DATA_VALUE];
}

// Unwraps the given symbol.
// Returns the symbol pointer.
int unwrap_symbol(int sym_ptr) {
  if (data[sym_ptr][DATA_TYPE] != DATA_TYPE_SYMBOL) panic(ERR_TYPE_MISMATCH);
  return data[sym_ptr][DATA_VALUE];
}

// Unwraps the given number.
// Returns the number value.
int unwrap_number(int num_ptr) {
  if (data[num_ptr][DATA_TYPE] != DATA_TYPE_NUMBER) panic(ERR_TYPE_MISMATCH);
  return data[num_ptr][DATA_VALUE];
}

// Evaluates the given data.
// Returns the pointer to the result data.
// Returns 0 if the evaluation has no result.
int eval(int data_ptr, int env_ptr) {
  // variable reference
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_SYMBOL) {
    int val_ptr = env_find(env_ptr, data[data_ptr][DATA_VALUE]);
    if (!val_ptr) panic(ERR_SYMBOL_NOT_FOUND);
    return copy_ptr(val_ptr);
  }
  // constant value
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_NUMBER) return copy_ptr(data_ptr);
  // list
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_LIST) {
    // list must have at least one element
    int head_ptr = data[data_ptr][DATA_VALUE];
    if (!head_ptr) panic(ERR_INVALID_LIST);
    int arg_ptr = data[head_ptr][DATA_NEXT];
    // check if the head is a user-defined symbol, or not a symbol
    if (data[head_ptr][DATA_TYPE] != DATA_TYPE_SYMBOL ||
        env_find(env_ptr, data[head_ptr][DATA_VALUE])) {
      // evaluate the head
      int func_ptr = eval(head_ptr, env_ptr);
      if (!func_ptr || data[func_ptr][DATA_TYPE] != DATA_TYPE_FUNC) {
        panic(ERR_INVALID_FUNC);
      }
      // get parameter list, body, and environment
      int param_list_ptr = data[func_ptr][DATA_VALUE];
      int body_ptr = data[param_list_ptr][DATA_NEXT];
      int outer_ptr = data[body_ptr][DATA_NEXT];
      // make function environment
      int func_env_ptr = make_env(copy_ptr(outer_ptr));
      // evaluate arguments
      int cur_param_ptr = data[param_list_ptr][DATA_VALUE];
      int cur_arg_ptr = arg_ptr;
      while (cur_param_ptr && cur_arg_ptr) {
        int param_sym_ptr = data[cur_param_ptr][DATA_VALUE];
        int arg_ptr = eval(cur_arg_ptr, env_ptr);
        env_add(func_env_ptr, param_sym_ptr, arg_ptr);
        cur_param_ptr = data[cur_param_ptr][DATA_NEXT];
        cur_arg_ptr = data[cur_arg_ptr][DATA_NEXT];
      }
      if (cur_param_ptr || cur_arg_ptr) panic(ERR_INVALID_ARG_NUM);
      // evaluate body
      int result_ptr = eval(body_ptr, func_env_ptr);
      // free function environment and function
      free_data(func_env_ptr);
      free_data(func_ptr);
      return result_ptr;
    }
    // handle built-in functions
    int sym_ptr = data[head_ptr][DATA_VALUE];
    // quote
    if (sym_ptr == SYM_QUOTE) {
      if (!arg_ptr || data[arg_ptr][DATA_NEXT]) panic(ERR_INVALID_ARG_NUM);
      return copy_ptr(arg_ptr);
    }
    // atom?
    if (sym_ptr == SYM_ATOM) {
      if (!arg_ptr || data[arg_ptr][DATA_NEXT]) panic(ERR_INVALID_ARG_NUM);
      int value_ptr = eval(arg_ptr, env_ptr);
      int result_ptr = make_bool(data[value_ptr][DATA_TYPE] != DATA_TYPE_LIST);
      free_data(value_ptr);
      return result_ptr;
    }
    // number?
    if (sym_ptr == SYM_NUMBER) {
      if (!arg_ptr || data[arg_ptr][DATA_NEXT]) panic(ERR_INVALID_ARG_NUM);
      int value_ptr = eval(arg_ptr, env_ptr);
      int result_ptr =
          make_bool(data[value_ptr][DATA_TYPE] == DATA_TYPE_NUMBER);
      free_data(value_ptr);
      return result_ptr;
    }
    // eq?
    if (sym_ptr == SYM_EQ) {
      int arg2_ptr = data[arg_ptr][DATA_NEXT];
      if (!arg_ptr || !arg2_ptr || data[arg2_ptr][DATA_NEXT]) {
        panic(ERR_INVALID_ARG_NUM);
      }
      int value1_ptr = eval(arg_ptr, env_ptr);
      int value2_ptr = eval(arg2_ptr, env_ptr);
      int result_ptr = make_bool(is_equal(value1_ptr, value2_ptr));
      free_data(value1_ptr);
      free_data(value2_ptr);
      return result_ptr;
    }
    // car
    if (sym_ptr == SYM_CAR) {
      if (!arg_ptr || data[arg_ptr][DATA_NEXT]) panic(ERR_INVALID_ARG_NUM);
      int value_ptr = eval(arg_ptr, env_ptr);
      int result_ptr = copy_ptr(unwrap_list(value_ptr));
      if (!result_ptr) panic(ERR_INVALID_LIST);
      free_data(value_ptr);
      return result_ptr;
    }
    // cdr
    if (sym_ptr == SYM_CDR) {
      if (!arg_ptr || data[arg_ptr][DATA_NEXT]) panic(ERR_INVALID_ARG_NUM);
      int value_ptr = eval(arg_ptr, env_ptr);
      int head_ptr = unwrap_list(value_ptr);
      if (!head_ptr) panic(ERR_INVALID_LIST);
      int result_ptr = make_list(copy_ptr(data[head_ptr][DATA_NEXT]));
      free_data(value_ptr);
      return result_ptr;
    }
    // cons
    if (sym_ptr == SYM_CONS) {
      int arg2_ptr = data[arg_ptr][DATA_NEXT];
      if (!arg_ptr || !arg2_ptr || data[arg2_ptr][DATA_NEXT]) {
        panic(ERR_INVALID_ARG_NUM);
      }
      int value1_ptr = eval(arg_ptr, env_ptr);
      int value2_ptr = eval(arg2_ptr, env_ptr);
      int head_ptr = copy_data(value1_ptr);
      data[head_ptr][DATA_NEXT] = copy_ptr(unwrap_list(value2_ptr));
      int result_ptr = make_list(head_ptr);
      free_data(value1_ptr);
      free_data(value2_ptr);
      return result_ptr;
    }
    // cond
    if (sym_ptr == SYM_COND) {
      int cur_ptr = arg_ptr;
      while (cur_ptr) {
        int test_ptr = unwrap_list(cur_ptr);
        int result_ptr = eval(test_ptr, env_ptr);
        if (is_true(result_ptr)) {
          int body_ptr = data[test_ptr][DATA_NEXT];
          int body_result_ptr = eval(body_ptr, env_ptr);
          free_data(result_ptr);
          return body_result_ptr;
        }
        free_data(result_ptr);
        cur_ptr = data[cur_ptr][DATA_NEXT];
      }
      return 0;
    }
    // lambda
    if (sym_ptr == SYM_LAMBDA) {
      int arg2_ptr = data[arg_ptr][DATA_NEXT];
      if (!arg_ptr || !arg2_ptr || data[arg2_ptr][DATA_NEXT]) {
        panic(ERR_INVALID_ARG_NUM);
      }
      int result_ptr =
          make_func(copy_data(arg_ptr), copy_data(arg2_ptr), copy_ptr(env_ptr));
      return result_ptr;
    }
    // define
    if (sym_ptr == SYM_DEFINE) {
      int arg2_ptr = data[arg_ptr][DATA_NEXT];
      if (!arg_ptr || !arg2_ptr || data[arg2_ptr][DATA_NEXT]) {
        panic(ERR_INVALID_ARG_NUM);
      }
      int sym_ptr = unwrap_symbol(arg_ptr);
      int value_ptr = eval(arg2_ptr, env_ptr);
      env_set(env_ptr, sym_ptr, value_ptr);
      return 0;
    }
    // list
    if (sym_ptr == SYM_LIST) {
      int result_ptr = make_list(0), cur_ptr = 0;
      while (arg_ptr) {
        int value_ptr = eval(arg_ptr, env_ptr);
        int elem_ptr = copy_data(value_ptr);
        free_data(value_ptr);
        if (cur_ptr) {
          data[cur_ptr][DATA_NEXT] = elem_ptr;
        } else {
          data[result_ptr][DATA_VALUE] = elem_ptr;
        }
        cur_ptr = elem_ptr;
        arg_ptr = data[arg_ptr][DATA_NEXT];
      }
      return result_ptr;
    }
    // math functions
    int arg2_ptr = data[arg_ptr][DATA_NEXT];
    if (!arg_ptr || !arg2_ptr || data[arg2_ptr][DATA_NEXT]) {
      panic(ERR_INVALID_ARG_NUM);
    }
    int value1_ptr = eval(arg_ptr, env_ptr);
    int value2_ptr = eval(arg2_ptr, env_ptr);
    int lhs = unwrap_number(value1_ptr), rhs = unwrap_number(value2_ptr);
    int result_ptr = 0;
    if (sym_ptr == SYM_ADD) {
      result_ptr = make_number(lhs + rhs);
    } else if (sym_ptr == SYM_SUB) {
      result_ptr = make_number(lhs - rhs);
    } else if (sym_ptr == SYM_MUL) {
      result_ptr = make_number(lhs * rhs);
    } else if (sym_ptr == SYM_DIV) {
      result_ptr = make_number(lhs / rhs);
    } else if (sym_ptr == SYM_LT) {
      result_ptr = make_bool(lhs < rhs);
    } else if (sym_ptr == SYM_LE) {
      result_ptr = make_bool(lhs <= rhs);
    } else if (sym_ptr == SYM_GT) {
      result_ptr = make_bool(lhs > rhs);
    } else if (sym_ptr == SYM_GE) {
      result_ptr = make_bool(lhs >= rhs);
    } else if (sym_ptr == SYM_EQ_NUM) {
      result_ptr = make_bool(lhs == rhs);
    } else {
      panic(ERR_INVALID_SYMBOL);
    }
    free_data(value1_ptr);
    free_data(value2_ptr);
    return result_ptr;
  }
  return panic(ERR_INVALID_DATA_TYPE);
}

// ======================================================================
// Printing
// ======================================================================

// Prints the given data.
void print(int data_ptr) {
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_SYMBOL) {
    print_sym(data[data_ptr][DATA_VALUE]);
    return;
  }
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_NUMBER) {
    putint(data[data_ptr][DATA_VALUE]);
    return;
  }
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_LIST) {
    putch(40);
    int list_ptr = data[data_ptr][DATA_VALUE];
    while (list_ptr) {
      print(list_ptr);
      list_ptr = data[list_ptr][DATA_NEXT];
      if (list_ptr) putch(32);
    }
    putch(41);
    return;
  }
  if (data[data_ptr][DATA_TYPE] == DATA_TYPE_FUNC) {
    putch(35);
    putch(60);
    putch(102);
    putch(117);
    putch(110);
    putch(99);
    putch(32);
    putint(data_ptr);
    putch(62);
    return;
  }
  panic(ERR_INVALID_DATA_TYPE);
}

// ======================================================================
// Main
// ======================================================================

int main() {
  // initialize data storage
  init_data();
  // makes the global environment
  int global_env_ptr = make_env(0);
  // read the first token
  last_token_type = next_token();
  // run the REPL
  while (1) {
    // parse the next data
    int data_ptr = parse();
    if (data_ptr == 0) break;
    // evaluate the data
    int result_ptr = eval(data_ptr, global_env_ptr);
    // print the result
    if (result_ptr) {
      print(result_ptr);
      putch(10);
    }
    // free the data and the result
    free_data(data_ptr);
    free_data(result_ptr);
  }
  return 0;
}