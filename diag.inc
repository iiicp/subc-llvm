#ifndef DIAG
#define DIAG(ID, KIND, MSG)
#endif

/// lexer
DIAG(err_unknown_char, Error, "unknown char '{0}'")
DIAG(err_unclosed_character, Error, "unclosed character literal")
DIAG(err_unclosed_string, Error, "unclosed string literal")
DIAG(err_numeric_constant, Error, "invalid numeric constant")

/// parser
DIAG(err_expected, Error, "expected '{0}', but found '{1}'")
DIAG(err_break_stmt, Error, "'break' statement not in loop or switch statement")
DIAG(err_continue_stmt, Error, "'continue' statement not in loop or switch statement")
DIAG(err_case_stmt, Error, "'case' statement not in switch statement")
DIAG(err_default_stmt, Error, "'default' statement not in switch statement")
DIAG(err_multi_default_stmt, Error, "only one 'default' statement in switch statement")

/// sema
DIAG(err_redefined, Error, "redefined symbol '{0}")
DIAG(err_undefined, Error, "undefined symbol '{0}")
DIAG(err_lvalue, Error, "required lvalue on the assign operation left side")
DIAG(err_type, Error, "typename expected")
DIAG(err_same_type, Error, "expected same type")
DIAG(err_expected_ype, Error, "expected {0}")
DIAG(err_expected_lvalue, Error, "expected lvalue")
DIAG(err_expected_ex, Error, "expected '{0}'")
DIAG(err_miss, Error, "miss '{0}'")
DIAG(err_large_length, Error, "large length")
DIAG(err_binary_expr_type, Error, "The types of the left and right sides of the binary expression do not support the operation")

DIAG(err_constant_expr, Error, "expect constant expr")
DIAG(err_int_constant_expr, Error, "expect int constant expr")
DIAG(err_arr_size, Error, "array size must be greater than 0")

#undef DIAG