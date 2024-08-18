#pragma once
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include "type.h"
#include "diag_engine.h"

/// char stream -> Token

enum class TokenType : uint8_t{
    number,
    identifier,
    kw_int,         // int
    kw_if,          // if
    kw_else,        // else
    kw_for,         // for
    kw_break,       // break
    kw_continue,    // continue
    kw_sizeof,      // sizeof
    minus,          // -
    plus,           // +
    star,           // *
    slash,          // '/'
    percent,        // %
    l_parent,       // (
    r_parent,       // )
    semi,           // ';'
    equal,          // =
    comma,          // ,
    l_brace,        // {
    r_brace,        // }
    equal_equal,    // ==
    not_equal,      // !=
    less,           // <
    less_equal,     // <=
    greater,        // >
    greater_equal,  // >=
    pipepipe,       // ||
    pipe,           // |
    amp,            // &
    ampamp,         // &&
    less_less,      // <<
    greater_greater,// >>
    caret,          // ^
    plus_plus,      // ++
    minus_minus,    // --
    tilde,          // ~
    exclaim,        // !
    plus_equal,     // +=
    minus_equal,    // -=
    star_equal,     // *=
    slash_equal,    // '/='
    percent_equal,  // %=
    less_less_equal, // <<=
    greater_greater_equal, // >>=
    amp_equal,      // &=
    caret_equal,    // ^=
    pipe_equal,     // |=
    question,       // ?
    colon,          // :
    l_bracket,      // [
    r_bracket,      // ]
    kw_struct,      // struct,
    kw_union,       // union
    dot,            // .
    arrow,          // ->
    kw_return,      // return
    eof             // end
};

class Token {
public:
    TokenType tokenType;
    int row, col;
    
    int value; // for number
    
    const char *ptr; // for debug && diag
    int len;

    std::shared_ptr<CType> ty; // for built-in type

    void Dump() {
        llvm::outs() << "{ " << llvm::StringRef(ptr, len) << ", row = " << row << ", col = " << col << "}\n";
    }

    static llvm::StringRef GetSpellingText(TokenType tokenType);
};

class Lexer {
private:
    llvm::SourceMgr &mgr;
    DiagEngine &diagEngine;
    llvm::StringRef fileName;
public:
    Lexer(llvm::SourceMgr &mgr, DiagEngine &diagEngine) : mgr(mgr), diagEngine(diagEngine) {
        unsigned id = mgr.getMainFileID();
        llvm::StringRef buf = mgr.getMemoryBuffer(id)->getBuffer();
        BufPtr = buf.begin();
        LineHeadPtr = buf.begin();
        BufEnd = buf.end();
        row = 1;
        fileName = mgr.getMemoryBuffer(id)->getBufferIdentifier();
    }

    void NextToken(Token &tok);

    void SaveState();
    void RestoreState();

    DiagEngine &GetDiagEngine() const {
        return diagEngine;
    }

    llvm::StringRef GetFileName() {
        return fileName;
    }
private:
    bool StartWith(const char *p);
private:
    const char *BufPtr;
    const char *LineHeadPtr;
    const char *BufEnd;
    int row;

    struct State{
        const char *BufPtr;
        const char *LineHeadPtr;
        const char *BufEnd;
        int row;
    };

    State state;
};