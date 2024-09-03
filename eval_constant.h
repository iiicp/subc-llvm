#pragma once
#include "ast.h"
#include <variant>
#include "diag_engine.h"

class EvalConstant{
private:
    DiagEngine &diagEngine;
public:
    using Constant = std::variant<int64_t, double>;

    EvalConstant(DiagEngine &diagEngine):diagEngine(diagEngine){}
    Constant Eval(AstNode *node);
private:
    Constant VisitNumberExpr(NumberExpr *expr);
    Constant VisitBinaryExpr(BinaryExpr *binaryExpr);
    Constant VisitUnaryExpr(UnaryExpr *expr);
    Constant VisitCastExpr(CastExpr *expr);
    Constant VisitSizeOfExpr(SizeOfExpr *expr);
    Constant VisitPostIncExpr(PostIncExpr *expr);
    Constant VisitPostDecExpr(PostDecExpr *expr);
    Constant VisitPostSubscript(PostSubscript *expr);
    Constant VisitPostMemberDotExpr(PostMemberDotExpr *expr);
    Constant VisitPostMemberArrowExpr(PostMemberArrowExpr *expr);
    Constant VisitThreeExpr(ThreeExpr *expr);
};