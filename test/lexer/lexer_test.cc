#include <gtest/gtest.h>
#include "lexer.h"
#include <functional>

bool TestLexerWithContent(llvm::StringRef content, std::function<std::vector<Token>()> callback) {
    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buf = llvm::MemoryBuffer::getMemBuffer(content, "stdin");
     if (!buf) {
        llvm::errs() << "open file failed!!!\n";
        return false;
    }
    llvm::SourceMgr mgr;
    DiagEngine diagEngine(mgr);
    mgr.AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());
    Lexer lexer(mgr, diagEngine);

    std::vector<Token> expectedVec = callback(), curVec;

    Token tok;
    while (true) {
        lexer.NextToken(tok);
        if (tok.tokenType == TokenType::eof)
            break;
        curVec.push_back(tok);
    }

    EXPECT_EQ(expectedVec.size(), curVec.size());
    for (int i = 0; i < expectedVec.size(); i++) {
        const auto &expected_tok = expectedVec[i];
        const auto &cur_tok = curVec[i];

        EXPECT_EQ(static_cast<uint8_t>(expected_tok.tokenType), static_cast<uint8_t>(cur_tok.tokenType));
        EXPECT_EQ(expected_tok.row, cur_tok.row);
        EXPECT_EQ(expected_tok.col, cur_tok.col);
    }
    return true;
}



TEST(LexerTest, identifier) {
    
    bool res = TestLexerWithContent("aaaa aA_ aA0_", []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::identifier, 1, 1});
        expectedVec.push_back(Token{TokenType::identifier, 1, 6});
        expectedVec.push_back(Token{TokenType::identifier, 1, 10});
        return expectedVec;
    });
    ASSERT_EQ(res, true);
}

TEST(LexerTest, keyword) {
    bool res = TestLexerWithContent("  int if else for break continue sizeof", []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::kw_int, 1, 3});
        expectedVec.push_back(Token{TokenType::kw_if, 1, 7});
        expectedVec.push_back(Token{TokenType::kw_else, 1, 10});
        expectedVec.push_back(Token{TokenType::kw_for, 1, 15});
        expectedVec.push_back(Token{TokenType::kw_break, 1, 19});
        expectedVec.push_back(Token{TokenType::kw_continue, 1, 25});
        expectedVec.push_back(Token{TokenType::kw_sizeof, 1, 34});
        return expectedVec;
    });
    ASSERT_EQ(res, true);
}

TEST(LexerTest, number) {
    bool res = TestLexerWithContent(" 0123 1234 1234222 \n0" , []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::number, 1, 2});
        expectedVec.push_back(Token{TokenType::number, 1, 7});
        expectedVec.push_back(Token{TokenType::number, 1, 12});
        expectedVec.push_back(Token{TokenType::number, 2, 1});
        return expectedVec;
    });
    ASSERT_EQ(res, true);
}

TEST(LexerTest, punctuation) {
    bool res = TestLexerWithContent("+-*/%();,={}==!=< <=> >= || | & && >><<^", []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::plus, 1, 1});
        expectedVec.push_back(Token{TokenType::minus, 1, 2});
        expectedVec.push_back(Token{TokenType::star, 1, 3});
        expectedVec.push_back(Token{TokenType::slash, 1, 4});
        expectedVec.push_back(Token{TokenType::percent, 1, 5});
        expectedVec.push_back(Token{TokenType::l_parent, 1, 6});
        expectedVec.push_back(Token{TokenType::r_parent, 1, 7});
        expectedVec.push_back(Token{TokenType::semi, 1, 8});
        expectedVec.push_back(Token{TokenType::comma, 1, 9});
        expectedVec.push_back(Token{TokenType::equal, 1, 10});
        expectedVec.push_back(Token{TokenType::l_brace, 1, 11});
        expectedVec.push_back(Token{TokenType::r_brace, 1, 12});
        expectedVec.push_back(Token{TokenType::equal_equal, 1, 13});
        expectedVec.push_back(Token{TokenType::not_equal, 1, 15});
        expectedVec.push_back(Token{TokenType::less, 1, 17});
        expectedVec.push_back(Token{TokenType::less_equal, 1, 19});
        expectedVec.push_back(Token{TokenType::greater, 1, 21});
        expectedVec.push_back(Token{TokenType::greater_equal, 1, 23});
        expectedVec.push_back(Token{TokenType::pipepipe, 1, 26});
        expectedVec.push_back(Token{TokenType::pipe, 1, 29});
        expectedVec.push_back(Token{TokenType::amp, 1, 31});
        expectedVec.push_back(Token{TokenType::ampamp, 1,33});
        expectedVec.push_back(Token{TokenType::greater_greater, 1, 36});
        expectedVec.push_back(Token{TokenType::less_less, 1, 38});
        expectedVec.push_back(Token{TokenType::caret, 1, 40});
        return expectedVec;
    });
    ASSERT_EQ(res, true);
}

TEST(LexerTest, unary) {
    bool res = TestLexerWithContent("+-*&++--!~+=-=*=/=%=<<=>>=&=^=|=?:", []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::plus, 1, 1});
        expectedVec.push_back(Token{TokenType::minus, 1, 2});
        expectedVec.push_back(Token{TokenType::star, 1, 3});
        expectedVec.push_back(Token{TokenType::amp, 1, 4});
        expectedVec.push_back(Token{TokenType::plus_plus, 1, 5});
        expectedVec.push_back(Token{TokenType::minus_minus, 1, 7});
        expectedVec.push_back(Token{TokenType::exclaim, 1, 9});
        expectedVec.push_back(Token{TokenType::tilde, 1, 10});

        expectedVec.push_back(Token{TokenType::plus_equal, 1, 11});
        expectedVec.push_back(Token{TokenType::minus_equal, 1, 13});
        expectedVec.push_back(Token{TokenType::star_equal, 1, 15});
        expectedVec.push_back(Token{TokenType::slash_equal, 1, 17});
        expectedVec.push_back(Token{TokenType::percent_equal, 1, 19});
        expectedVec.push_back(Token{TokenType::less_less_equal, 1, 21});
        expectedVec.push_back(Token{TokenType::greater_greater_equal, 1, 24});
        expectedVec.push_back(Token{TokenType::amp_equal, 1, 27});
        expectedVec.push_back(Token{TokenType::caret_equal, 1, 29});
        expectedVec.push_back(Token{TokenType::pipe_equal, 1, 31});
        expectedVec.push_back(Token{TokenType::question, 1, 33});
        expectedVec.push_back(Token{TokenType::colon, 1, 34});
        return expectedVec;
    });
}

TEST(LexerTest, expr_01) {
    bool res = TestLexerWithContent("1+3; 2+(4*2)/2+4;\n5-3*2;", []()->std::vector<Token> {
        std::vector<Token> expectedVec;
        expectedVec.push_back(Token{TokenType::number, 1, 1});
        expectedVec.push_back(Token{TokenType::plus, 1, 2});
        expectedVec.push_back(Token{TokenType::number, 1, 3});
        expectedVec.push_back(Token{TokenType::semi, 1, 4});
        expectedVec.push_back(Token{TokenType::number, 1, 6});
        expectedVec.push_back(Token{TokenType::plus, 1, 7});
        expectedVec.push_back(Token{TokenType::l_parent, 1, 8});
        expectedVec.push_back(Token{TokenType::number, 1, 9});

        expectedVec.push_back(Token{TokenType::star, 1, 10});
        expectedVec.push_back(Token{TokenType::number, 1, 11});
        expectedVec.push_back(Token{TokenType::r_parent, 1, 12});
        expectedVec.push_back(Token{TokenType::slash, 1, 13});
        expectedVec.push_back(Token{TokenType::number, 1, 14});
        expectedVec.push_back(Token{TokenType::plus, 1, 15});
        expectedVec.push_back(Token{TokenType::number, 1, 16});
        expectedVec.push_back(Token{TokenType::semi, 1, 17});
        expectedVec.push_back(Token{TokenType::number, 2, 1});
        expectedVec.push_back(Token{TokenType::minus, 2, 2});
        expectedVec.push_back(Token{TokenType::number, 2, 3});
        expectedVec.push_back(Token{TokenType::star, 2, 4});
        expectedVec.push_back(Token{TokenType::number, 2, 5});
        expectedVec.push_back(Token{TokenType::semi, 2, 6});
        return expectedVec;
    });
}