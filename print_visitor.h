#pragma once
#include "ast.h"
#include "parser.h"
class PrintVisitor : public Visitor, public TypeVisitor {
private:
    llvm::raw_ostream *out;
public:
    PrintVisitor(std::shared_ptr<Program> program, llvm::raw_ostream *out = &llvm::outs());

    llvm::Value * VisitProgram(Program *p) override;
    llvm::Value * VisitBlockStmt(BlockStmt *p) override;
    llvm::Value * VisitDeclStmt(DeclStmt *p) override;
    llvm::Value * VisitIfStmt(IfStmt *p) override;
    llvm::Value * VisitForStmt(ForStmt *p) override;
    llvm::Value * VisitContinueStmt(ContinueStmt *p) override;
    llvm::Value * VisitReturnStmt(ReturnStmt *p) override;
    llvm::Value * VisitBreakStmt(BreakStmt *p) override;
    llvm::Value * VisitSwitchStmt(SwitchStmt *p) override;
    llvm::Value * VisitCaseStmt(CaseStmt *p) override;
    llvm::Value * VisitDefaultStmt(DefaultStmt *p) override;
    llvm::Value * VisitDoWhileStmt(DoWhileStmt *p) override;
    llvm::Value * VisitVariableDecl(VariableDecl *decl) override;
    llvm::Value * VisitFuncDecl(FuncDecl *decl) override;
    llvm::Value * VisitNumberExpr(NumberExpr *expr) override;
    llvm::Value * VisitStringExpr(StringExpr *expr) override;
    llvm::Value * VisitBinaryExpr(BinaryExpr *binaryExpr) override;
    llvm::Value * VisitUnaryExpr(UnaryExpr *expr) override;
    llvm::Value * VisitCastExpr(CastExpr *expr) override;
    llvm::Value * VisitSizeOfExpr(SizeOfExpr *expr) override;
    llvm::Value * VisitPostIncExpr(PostIncExpr *expr) override;
    llvm::Value * VisitPostDecExpr(PostDecExpr *expr) override;
    llvm::Value * VisitPostSubscript(PostSubscript *expr) override;
    llvm::Value * VisitPostMemberDotExpr(PostMemberDotExpr *expr) override;
    llvm::Value * VisitPostMemberArrowExpr(PostMemberArrowExpr *expr) override;
    llvm::Value * VisitPostFuncCall(PostFuncCall *expr) override;
    llvm::Value * VisitThreeExpr(ThreeExpr *expr) override;
    llvm::Value * VisitVariableAccessExpr(VariableAccessExpr *factorExpr) override;

    llvm::Type * VisitPrimaryType(CPrimaryType *ty) override;
    llvm::Type * VisitPointType(CPointType *ty) override;
    llvm::Type * VisitArrayType(CArrayType *ty) override;
    llvm::Type * VisitRecordType(CRecordType *ty) override;
    llvm::Type * VisitFuncType(CFuncType *ty) override;
};