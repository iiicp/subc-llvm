#pragma once
#include "scope.h"
#include "ast.h"
#include "diag_engine.h"
class Sema {
public:
    enum class Mode {
        Normal,
        Skip
    };
private:
    DiagEngine &diagEngine;
public:
    Sema(DiagEngine &diagEngine):diagEngine(diagEngine) {}
    std::shared_ptr<AstNode> SemaVariableDeclNode(Token tok, std::shared_ptr<CType> ty, bool isGlobal);
    std::shared_ptr<AstNode> SemaVariableAccessNode(Token tok);
    std::shared_ptr<AstNode> SemaNumberExprNode(Token tok, int val, std::shared_ptr<CType> ty);
    std::shared_ptr<AstNode> SemaStringExprNode(Token tok, std::string val, std::shared_ptr<CType> ty);
    std::shared_ptr<AstNode> SemaBinaryExprNode( std::shared_ptr<AstNode> left,std::shared_ptr<AstNode> right, BinaryOp op);

    std::shared_ptr<AstNode> SemaUnaryExprNode( std::shared_ptr<AstNode> unary, UnaryOp op, Token tok);
    std::shared_ptr<AstNode> SemaThreeExprNode( std::shared_ptr<AstNode> cond,std::shared_ptr<AstNode> then, std::shared_ptr<AstNode> els, Token tok);
    std::shared_ptr<AstNode> SemaSizeofExprNode( std::shared_ptr<AstNode> unary,std::shared_ptr<CType> ty);
    std::shared_ptr<AstNode> SemaPostIncExprNode( std::shared_ptr<AstNode> left, Token tok);
    std::shared_ptr<AstNode> SemaPostDecExprNode( std::shared_ptr<AstNode> left, Token tok);
    std::shared_ptr<AstNode> SemaPostSubscriptNode(std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> node, Token tok);

    std::shared_ptr<AstNode> SemaPostMemberDotNode(std::shared_ptr<AstNode> left, Token iden, Token dotTok);
    std::shared_ptr<AstNode> SemaPostMemberArrowNode(std::shared_ptr<AstNode> left, Token iden, Token arrowTok);

    std::shared_ptr<VariableDecl::InitValue> SemaDeclInitValue(std::shared_ptr<CType> declType, std::shared_ptr<AstNode> value, std::vector<int> &offsetList, Token tok);
    std::shared_ptr<AstNode> SemaIfStmtNode(std::shared_ptr<AstNode> condNode, std::shared_ptr<AstNode> thenNode, std::shared_ptr<AstNode> elseNode);

    std::shared_ptr<CType> SemaTagAccess(Token tok);
    std::shared_ptr<CType> SemaTagDecl(Token tok, const std::vector<Member> &members, TagKind tagKind);
    std::shared_ptr<CType> SemaTagDecl(Token tok, std::shared_ptr<CType> type);
    std::shared_ptr<CType> SemaAnonyTagDecl(const std::vector<Member> &members, TagKind tagKind);

    std::shared_ptr<AstNode> SemaFuncDecl(Token tok, std::shared_ptr<CType> type, std::shared_ptr<AstNode> blockStmt);
    std::shared_ptr<AstNode> SemaFuncCall(std::shared_ptr<AstNode> left, const std::vector<std::shared_ptr<AstNode>> &args);

    void EnterScope();
    void ExitScope();
    void SetMode(Mode mode);
private:
    Scope scope;
    Mode mode{Mode::Normal};
};