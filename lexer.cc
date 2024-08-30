#include "lexer.h"
#include <cstring>

llvm::StringRef Token::GetSpellingText(TokenType tokenType) {
    switch (tokenType)
    {
    case TokenType::number:
        return "number";
    case TokenType::str:
        return "string";
    case TokenType::identifier:
        return "identifier";
    case TokenType::kw_int:
        return "int";
    case TokenType::kw_if:
        return "if";
    case TokenType::kw_else:
        return "else";
    case TokenType::kw_for:
        return "for";
    case TokenType::kw_break:
        return "break";
    case TokenType::kw_continue:
        return "continue";
    case TokenType::kw_sizeof:
        return "sizeof";
    case TokenType::minus:
        return "-";
    case TokenType::plus:
        return "+";
    case TokenType::star:
        return "*";
    case TokenType::slash:
        return "/";
    case TokenType::percent:
        return "%";
    case TokenType::l_parent:
        return "(";
    case TokenType::r_parent:
        return ")";
    case TokenType::semi:
        return ";";
    case TokenType::equal:
        return "=";
    case TokenType::comma:
        return ",";
    case TokenType::l_brace:
        return "{";
    case TokenType::r_brace:
        return "}";
    case TokenType::equal_equal:
        return "==";
    case TokenType::not_equal:
        return "!=";
    case TokenType::less:
        return "<";
    case TokenType::less_equal:
        return "<=";
    case TokenType::greater:
        return ">";
    case TokenType::greater_equal:
        return ">=";
    case TokenType::pipepipe:
        return "||";
    case TokenType::pipe:
        return "|";
    case TokenType::amp:
        return "&";
    case TokenType::ampamp:
        return "&&";
    case TokenType::less_less:
        return "<<";
    case TokenType::greater_greater:
        return ">>";
    case TokenType::caret:
        return "^";
    case TokenType::plus_plus:
        return "++";
    case TokenType::minus_minus:
        return "--";
    case TokenType::tilde:
        return "~";
    case TokenType::exclaim:
        return "!";
    case TokenType::plus_equal:
        return "+=";
    case TokenType::minus_equal:
        return "-=";
    case TokenType::star_equal:
        return "*=";
    case TokenType::slash_equal:
        return "/=";
    case TokenType::percent_equal:
        return "%=";
    case TokenType::less_less_equal:
        return "<<=";
    case TokenType::greater_greater_equal:
        return ">>=";
    case TokenType::amp_equal:
        return "&=";
    case TokenType::caret_equal:
        return "^=";
    case TokenType::pipe_equal:
        return "|=";
    case TokenType::question:
        return "?";
    case TokenType::colon:
        return ":";
    case TokenType::l_bracket:
        return "[";
    case TokenType::r_bracket:
        return "]";
    case TokenType::kw_struct:
        return "struct";
    case TokenType::kw_union:
        return "union";
    case TokenType::dot:
        return ".";
    case TokenType::arrow:
        return "->";
    case TokenType::kw_return:
        return "return";
    case TokenType::kw_void:
        return "void";
    case TokenType::kw_const:
        return "const";
    case TokenType::kw_volatile:
        return "volatile";
    case TokenType::kw_static:
        return "static";
    case TokenType::ellipse:
        return "...";
    case TokenType::kw_while:
        return "while";
    case TokenType::kw_do:
        return "do";
    case TokenType::kw_switch:
        return "switch";
    case TokenType::kw_case:
        return "case";
    case TokenType::kw_default:
        return "default";
    case TokenType::kw_short:
        return "short";
    case TokenType::kw_long:
        return "long";
    case TokenType::kw_float:
        return "float";
    case TokenType::kw_double:
        return "double";
    case TokenType::kw_signed:
        return "signed";
    case TokenType::kw_unsigned:
        return "unsigned";
    case TokenType::kw_typedef:
        return "typedef";
    case TokenType::kw_extern:
        return "extern";
    case TokenType::kw_auto:
        return "auto";
    case TokenType::kw_register:
        return "register";
    case TokenType::kw_inline:
        return "inline";
    default:
        llvm::llvm_unreachable_internal();
    }
}

bool IsWhiteSpace(char ch) {
    return ch == ' ' || ch == '\r' || ch == '\n' || ch == '\t';
}

bool IsDigit(char ch) {
    return (ch >= '0' && ch <= '9');
}

bool IsHexDigit(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

bool IsLetter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch == '_');
}

// Read a single character in a char or string literal.
static const char *c_char(int *res, const char *p) {
  // Nonescaped
  if (*p != '\\') {
    *res = *p;
    return p + 1;
  }
  p++;

  int esc = 0;
  switch (*p) {
    case 'a': {
        esc = '\a';
        break;
    }
    case 'b': {
        esc = '\b';
        break;
    }
    case 'f': {
        esc = '\f';
        break;
    }
    case 'n': {
        esc = '\n';
        break;
    }
    case 'r': {
        esc = '\r';
        break;
    }
    case 't': {
        esc = '\t';
        break;
    }
    case 'v': {
        esc = '\v';
        break;
    }
    default : {
        esc = *p;
        break;
    }
  }
    *res = esc;
    return p + 1;
}

bool Lexer::StartWith(const char *p) {
    return !strncmp(BufPtr, p, strlen(p));
}

bool Lexer::StartWith(const char *source, const char *target) {
    return !strncmp(source, target, strlen(target));
}

void Lexer::NextToken(Token &tok) {
    
    /// 1. 过滤空格 
    while (IsWhiteSpace(*BufPtr) || StartWith("//") || StartWith("/*")) {
        if (StartWith("//")) {
            while (*BufPtr != '\n' && (BufPtr < BufEnd)){
                BufPtr++;
            }
        }

        if (StartWith("/*")) {
            while ((BufPtr[0] != '*' || BufPtr[1] != '/') && (BufPtr < BufEnd)) {
                if (*BufPtr == '\n') {
                    row++;
                    LineHeadPtr = BufPtr+1;
                }
                BufPtr++;
            }
            BufPtr += 2;
        }

        if (BufPtr >= BufEnd) {
            tok.tokenType = TokenType::eof;
            return;
        }

        if (*BufPtr == '\n') {
            row++;
            LineHeadPtr = BufPtr+1;
        }
        BufPtr++;
    }

    /// 2. 判断是否到结尾
    if (BufPtr >= BufEnd) {
        tok.tokenType = TokenType::eof;
        return;
    }

    tok.row = row;
    tok.col = BufPtr - LineHeadPtr + 1;

    const char *StartPtr = BufPtr;

    if (*BufPtr == '\'') {
        tok.tokenType = TokenType::number;
        tok.ty = CType::IntType;
        tok.ptr = BufPtr++;
        BufPtr = c_char((int *)&tok.value.v, BufPtr);
        if (*BufPtr != '\'')
            diagEngine.Report(llvm::SMLoc::getFromPointer(BufPtr), diag::err_unclosed_character);
        BufPtr += 1;
        tok.len = BufPtr - StartPtr;
    }
    else if (*BufPtr == '"') {
        BufPtr++; // skip "
        tok.tokenType = TokenType::str;
        tok.ptr = BufPtr;
        std::string value;
        while (*BufPtr != '"') {
            if (!*BufPtr) {
                diagEngine.Report(llvm::SMLoc::getFromPointer(BufPtr), diag::err_unclosed_string);
            }
            int c;
            BufPtr = c_char(&c, BufPtr);
            value += c;
        }
        BufPtr++; // skip "
        tok.len = BufPtr - tok.ptr;
        tok.strVal = value;
        tok.ty = std::make_shared<CArrayType>(CType::CharType, tok.len);
    }
    else if (StartWith("0x") || StartWith("0X") || 
            StartWith("0b") || StartWith("0B")  ||
            IsDigit(*BufPtr) || (*BufPtr == '.' && IsDigit(BufPtr[1]))) {
        char *p = (char *)BufPtr;
        for (;;) {
            if (p[0] && p[1] && strchr("eEpP", p[0]) && strchr("+-", p[1]))
                p += 2;
            else if (isalnum(*p) || *p == '.')
                p++;
            else
                break;
        }
        BufPtr = ConvertNumber(tok, StartPtr, p);
    } else if (IsLetter(*BufPtr)) {
        while (IsLetter(*BufPtr) || IsDigit(*BufPtr)) {
            BufPtr++;
        }
        tok.tokenType = TokenType::identifier;
        tok.ptr = StartPtr;
        tok.len = BufPtr - StartPtr;
        llvm::StringRef text = llvm::StringRef(tok.ptr, tok.len);
        if (text == "int") {
            tok.tokenType = TokenType::kw_int;
        }else if (text == "if") {
            tok.tokenType = TokenType::kw_if;
        }else if (text == "else") {
            tok.tokenType = TokenType::kw_else;
        }else if (text == "for") {
            tok.tokenType = TokenType::kw_for;
        }else if (text == "break") {
            tok.tokenType = TokenType::kw_break;
        }else if (text == "continue") {
            tok.tokenType = TokenType::kw_continue;
        }else if (text == "sizeof") {
            tok.tokenType = TokenType::kw_sizeof;
        }else if (text == "struct") {
            tok.tokenType = TokenType::kw_struct;
        }else if (text == "union") {
            tok.tokenType = TokenType::kw_union;
        }else if (text == "return") {
            tok.tokenType = TokenType::kw_return;
        }else if (text == "void") {
            tok.tokenType = TokenType::kw_void;
        }else if (text == "char") {
            tok.tokenType = TokenType::kw_char;
        }else if (text == "const") {
            tok.tokenType = TokenType::kw_const;
        }else if (text == "volatile") {
            tok.tokenType = TokenType::kw_volatile;
        }else if (text == "static") {
            tok.tokenType = TokenType::kw_static;
        }else if (text == "while") {
            tok.tokenType = TokenType::kw_while;
        }else if (text == "do") {
            tok.tokenType = TokenType::kw_do;
        }else if (text == "switch") {
            tok.tokenType = TokenType::kw_switch;
        }else if (text == "case") {
            tok.tokenType = TokenType::kw_case;
        }else if (text == "default") {
            tok.tokenType = TokenType::kw_default;
        }else if (text == "short") {
            tok.tokenType = TokenType::kw_short;
        }else if (text == "long") {
            tok.tokenType = TokenType::kw_long;
        }else if (text == "float") {
            tok.tokenType = TokenType::kw_float;
        }else if (text == "double") {
            tok.tokenType = TokenType::kw_double;
        }else if (text == "signed") {
            tok.tokenType = TokenType::kw_signed;
        }else if (text == "unsigned") {
            tok.tokenType = TokenType::kw_unsigned;
        }else if (text == "typedef") {
            tok.tokenType = TokenType::kw_typedef;
        }else if (text == "auto") {
            tok.tokenType = TokenType::kw_auto;
        }else if (text == "extern") {
            tok.tokenType = TokenType::kw_extern;
        }else if (text == "register") {
            tok.tokenType = TokenType::kw_register;
        }else if (text == "inline") {
            tok.tokenType = TokenType::kw_inline;
        }
    }
    else {
        switch (*BufPtr)
        {
        case '+': {
            if (BufPtr[1] == '+') {
                tok.tokenType = TokenType::plus_plus;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else if (BufPtr[1] == '=') {
                tok.tokenType = TokenType::plus_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }
            else {
                tok.tokenType = TokenType::plus;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '-': {
            if (BufPtr[1] == '-') {
                tok.tokenType = TokenType::minus_minus;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else if (BufPtr[1] == '=') {
                tok.tokenType = TokenType::minus_equal;
                BufPtr += 2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else if (BufPtr[1] == '>') {
                tok.tokenType = TokenType::arrow;
                BufPtr += 2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }
            else {
                tok.tokenType = TokenType::minus;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '*':{
            if (BufPtr[1] == '=') {
                tok.tokenType = TokenType::star_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;                
            }else {
                tok.tokenType = TokenType::star;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '/':{
            if (BufPtr[1] == '=') {
                tok.tokenType = TokenType::slash_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;                
            }else {            
                tok.tokenType = TokenType::slash;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '%':{
            if (BufPtr[1] == '=') {
                tok.tokenType = TokenType::percent_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;                
            }else {              
                tok.tokenType = TokenType::percent;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case ';':{
            tok.tokenType = TokenType::semi;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }
        case '(':{
            tok.tokenType = TokenType::l_parent;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }
        case ')':{
            tok.tokenType = TokenType::r_parent;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }   
        case '=':{
            if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::equal_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else {
                tok.tokenType = TokenType::equal;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        } 
        case '|':{
            if (*(BufPtr+1) == '|') {
                tok.tokenType = TokenType::pipepipe;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            } else if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::pipe_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }
            else {
                tok.tokenType = TokenType::pipe;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        } 
        case '&':{
            if (*(BufPtr+1) == '&') {
                tok.tokenType = TokenType::ampamp;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            } else if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::amp_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }
            else {
                tok.tokenType = TokenType::amp;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        } 
        case '~': {
            tok.tokenType = TokenType::tilde;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }             
        case ',':{
            tok.tokenType = TokenType::comma;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        } 
        case '{': {
            tok.tokenType = TokenType::l_brace;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }     
        case '}': {
            tok.tokenType = TokenType::r_brace;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }
        case '^': {
            if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::caret_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else {
                tok.tokenType = TokenType::caret;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '<': {
            if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::less_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else if (*(BufPtr+1) == '<') {
                if (BufPtr[2] == '=') {
                    tok.tokenType = TokenType::less_less_equal;
                    BufPtr+=3;
                    tok.ptr = StartPtr;
                    tok.len = 3;
                }else {
                    tok.tokenType = TokenType::less_less;
                    BufPtr+=2;
                    tok.ptr = StartPtr;
                    tok.len = 2;
                }
            }else {
                tok.tokenType = TokenType::less;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '>': {
            if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::greater_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else if (*(BufPtr+1) == '>') {
                if (BufPtr[2] == '=') {
                    tok.tokenType = TokenType::greater_greater_equal;
                    BufPtr+=3;
                    tok.ptr = StartPtr;
                    tok.len = 3;
                }else {
                    tok.tokenType = TokenType::greater_greater;
                    BufPtr+=2;
                    tok.ptr = StartPtr;
                    tok.len = 2;
                }
            }else {
                tok.tokenType = TokenType::greater;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }            
            break;
        }
        case '!': {
            if (*(BufPtr+1) == '=') {
                tok.tokenType = TokenType::not_equal;
                BufPtr+=2;
                tok.ptr = StartPtr;
                tok.len = 2;
            }else {
                tok.tokenType = TokenType::exclaim;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;
            }
            break;
        }
        case '?': {
            tok.tokenType = TokenType::question;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;
            break;
        }
        case ':': {
            tok.tokenType = TokenType::colon;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;            
            break;
        }
        case '[': {
            tok.tokenType = TokenType::l_bracket;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;            
            break;
        }
        case ']': {
            tok.tokenType = TokenType::r_bracket;
            BufPtr++;
            tok.ptr = StartPtr;
            tok.len = 1;            
            break;
        }
        case '.': {
            if (BufPtr[1] == '.' && BufPtr[2] == '.') {
                tok.tokenType = TokenType::ellipse;
                BufPtr+=3;
                tok.ptr = StartPtr;
                tok.len = 3;
            }else {
                tok.tokenType = TokenType::dot;
                BufPtr++;
                tok.ptr = StartPtr;
                tok.len = 1;            
            }
            break;
        }        
        default:
            diagEngine.Report(llvm::SMLoc::getFromPointer(BufPtr), diag::err_unknown_char, *BufPtr);
            break;
        }
    }
}

const char * Lexer::ConvertNumber(Token &tok, const char *start, const char *end) {
    const auto &[res, endptr] = ConvertIntNumber(tok, start, end);
    if (res) {
        return endptr;
    }
    return ConvertFloatNumber(tok, start, end).second;
}

std::pair<bool, const char *> Lexer::ConvertIntNumber(Token &tok, const char *start, const char *end) {
  // Read a binary, octal, decimal or hexadecimal number.
  char *p = (char *)start;
  int base = 10;
  if (!strncasecmp(p, "0x", 2) && isxdigit(p[2])) {
    p += 2;
    base = 16;
  } else if (!strncasecmp(p, "0b", 2) && (p[2] == '0' || p[2] == '1')) {
    p += 2;
    base = 2;
  } else if (*p == '0') {
    base = 8;
  }

  int64_t val = strtoul(p, &p, base);

  // Read U, L or LL suffixes.
  bool l = false;
  bool u = false;

  if (StartWith(p, "LLU") || StartWith(p, "LLu") ||
      StartWith(p, "llU") || StartWith(p, "llu") ||
      StartWith(p, "ULL") || StartWith(p, "Ull") ||
      StartWith(p, "uLL") || StartWith(p, "ull")) {
    p += 3;
    l = u = true;
  } else if (!strncasecmp(p, "lu", 2) || !strncasecmp(p, "ul", 2)) {
    p += 2;
    l = u = true;
  } else if (StartWith(p, "LL") || StartWith(p, "ll")) {
    p += 2;
    l = true;
  } else if (*p == 'L' || *p == 'l') {
    p++;
    l = true;
  } else if (*p == 'U' || *p == 'u') {
    p++;
    u = true;
  }

  if (p != end)
    return {false, p};

  // Infer a type.
  std::shared_ptr<CType> ty;
  if (base == 10) {
    if (l && u)
      ty = CType::ULongType;//ty_ulong;
    else if (l)
      ty = CType::LongType;//ty_long;
    else if (u)
      ty = (val >> 32) ? CType::ULongType : CType::UIntType;
    else
      ty = (val >> 31) ? CType::LongType : CType::IntType;
  } else {
    if (l && u)
      ty = CType::ULongType;
    else if (l)
      ty = (val >> 63) ? CType::ULongType : CType::LongType;
    else if (u)
      ty = (val >> 32) ? CType::ULongType : CType::UIntType;
    else if (val >> 63)
      ty = CType::ULongType;
    else if (val >> 32)
      ty = CType::LongType;
    else if (val >> 31)
      ty = CType::UIntType;
    else
      ty = CType::IntType;
  }

  tok.tokenType = TokenType::number;
  tok.ty = ty;
  tok.ptr = start;
  tok.len = end - start;
  tok.value.v = val;
  return {true, end};
}

std::pair<bool, const char *> Lexer::ConvertFloatNumber(Token &tok, const char *pstart, const char *pend) {
  // If it's not an integer, it must be a floating point constant.
  char *end;
  long double val = strtold(pstart, &end);

  std::shared_ptr<CType> ty;
  if (*end == 'f' || *end == 'F') {
    ty = CType::FloatType;
    end++;
  } else if (*end == 'l' || *end == 'L') {
    ty = CType::LDoubleType;
    end++;
  } else {
    ty = CType::DoubleType;
  }

  if (pend != end) {
    diagEngine.Report(llvm::SMLoc::getFromPointer(end), diag::err_numeric_constant);
    return {false, end};
  }

  tok.tokenType = TokenType::number;
  tok.value.d = val;
  tok.ty = ty;
  tok.ptr = pstart;
  tok.len = pend - pstart;
  return {true, pend};
}

void Lexer::SaveState() {
    State state;
    state.BufPtr = BufPtr;
    state.LineHeadPtr = LineHeadPtr;
    state.BufEnd = BufEnd;
    state.row = row;
    stateStack.push(state);
}

void Lexer::RestoreState() {
    State state = stateStack.top();
    stateStack.pop();
    BufPtr = state.BufPtr;
    LineHeadPtr = state.LineHeadPtr;
    BufEnd = state.BufEnd;
    row = state.row;
}