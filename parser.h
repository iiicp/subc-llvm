#pragma once
#include "lexer.h"
#include "ast.h"
#include "sema.h"
class Parser {
private:
    Lexer &lexer;
    Sema &sema;
    std::vector<std::shared_ptr<AstNode>> breakNodes;
    std::vector<std::shared_ptr<AstNode>> continueNodes;
public:
    Parser(Lexer &lexer, Sema &sema) : lexer(lexer), sema(sema) {
        Advance();
    }

    std::shared_ptr<Program> ParseProgram();

private:
    std::shared_ptr<AstNode> ParseFuncDecl();
    std::shared_ptr<AstNode> ParseStmt();
    std::shared_ptr<AstNode> ParseBlockStmt();
    std::shared_ptr<AstNode> ParseDeclStmt(bool isGlobal = false);
    std::shared_ptr<CType> ParseDeclSpec();
    std::shared_ptr<CType> ParseStructOrUnionSpec();
    std::shared_ptr<AstNode> Declarator(std::shared_ptr<CType> baseType, bool isGlobal);
    std::shared_ptr<AstNode> DirectDeclarator(std::shared_ptr<CType> baseType, bool isGlobal);
    std::shared_ptr<CType> DirectDeclaratorSuffix(Token iden, std::shared_ptr<CType> baseType, bool isGlobal);
    std::shared_ptr<CType> DirectDeclaratorArraySuffix(std::shared_ptr<CType> baseType, bool isGlobal);
    std::shared_ptr<CType> DirectDeclaratorFuncSuffix(Token iden, std::shared_ptr<CType> baseType, bool isGlobal);
    bool ParseInitializer(std::vector<std::shared_ptr<VariableDecl::InitValue>> &arr, std::shared_ptr<CType> declType, std::vector<int> &offsetList, bool hasLBrace);

    std::shared_ptr<AstNode> ParseIfStmt();
    std::shared_ptr<AstNode> ParseForStmt();
    std::shared_ptr<AstNode> ParseBreakStmt();
    std::shared_ptr<AstNode> ParseContinueStmt();
    std::shared_ptr<AstNode> ParseReturnStmt();
    std::shared_ptr<AstNode> ParseExprStmt();
    std::shared_ptr<AstNode> ParseExpr();
    std::shared_ptr<AstNode> ParseAssignExpr();
    std::shared_ptr<AstNode> ParseConditionalExpr();

    std::shared_ptr<AstNode> ParseLogOrExpr();
    std::shared_ptr<AstNode> ParseLogAndExpr();
    std::shared_ptr<AstNode> ParseBitOrExpr();
    std::shared_ptr<AstNode> ParseBitXorExpr();
    std::shared_ptr<AstNode> ParseBitAndExpr();
    std::shared_ptr<AstNode> ParseShiftExpr();

    std::shared_ptr<AstNode> ParseEqualExpr();
    std::shared_ptr<AstNode> ParseRelationalExpr();
    std::shared_ptr<AstNode> ParseAddExpr();
    std::shared_ptr<AstNode> ParseMultiExpr();
    std::shared_ptr<AstNode> ParseUnaryExpr();
    std::shared_ptr<AstNode> ParsePostFixExpr();
    std::shared_ptr<AstNode> ParsePrimary();

    std::shared_ptr<CType> ParseType();

    bool IsAssignOperator();
    bool IsUnaryOperator();

    bool IsTypeName(TokenType tokenType);

    bool IsFuncDecl();

    /// 消费 token 的函数
    /// 检测当前 token是否是该类型，不会消费
    bool Expect(TokenType tokenType);
    /// 检测，并消费
    bool Consume(TokenType tokenType);
    /// 前进一个 token
    void Advance();

    DiagEngine &GetDiagEngine() {
        return lexer.GetDiagEngine();
    }

    Token tok;
};