#ifndef DIAG
#define DIAG(ID, KIND, MSG)
#endif

/// lexer
DIAG(err_unknown_char, Error, "unknown char '{0}'")

/// parser
DIAG(err_expected, Error, "expected '{0}', but found '{1}'")
DIAG(err_break_stmt, Error, "'break' statement not in loop or switch statement")
DIAG(err_continue_stmt, Error, "'continue' statement not in loop or switch statement")

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

#undef DIAG