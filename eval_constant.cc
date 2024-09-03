#include "eval_constant.h"
#include "type.h"
EvalConstant::Constant EvalConstant::Eval(AstNode *node) {
    auto kind = node->GetKind();
    switch (kind)
    {
        case AstNode::ND_BinaryExpr: 
            return VisitBinaryExpr(llvm::dyn_cast<BinaryExpr>(node));
        case AstNode::ND_ThreeExpr:
            return VisitThreeExpr(llvm::dyn_cast<ThreeExpr>(node));
        case AstNode::ND_UnaryExpr:
            return VisitUnaryExpr(llvm::dyn_cast<UnaryExpr>(node));
        case AstNode::ND_CastExpr:
            return VisitCastExpr(llvm::dyn_cast<CastExpr>(node));
        case AstNode::ND_SizeOfExpr:
            return VisitSizeOfExpr(llvm::dyn_cast<SizeOfExpr>(node));
        case AstNode::ND_NumberExpr:
            return VisitNumberExpr(llvm::dyn_cast<NumberExpr>(node));
        default:
            break;
    }
    diagEngine.Report(llvm::SMLoc::getFromPointer(node->tok.ptr), diag::err_constant_expr);
    return 0;
}

EvalConstant::Constant EvalConstant::VisitNumberExpr(NumberExpr *expr) {
    if (expr->ty->IsIntegerType()) {
        return expr->value.v;
    }
    return expr->value.d;
}

EvalConstant::Constant EvalConstant::VisitBinaryExpr(BinaryExpr *binaryExpr) {
    EvalConstant::Constant left = Eval(binaryExpr->left.get());
    EvalConstant::Constant right = Eval(binaryExpr->right.get());
    return std::visit([&](auto &lhs) -> EvalConstant::Constant {
        using T1 = std::decay_t<decltype(lhs)>;
        return std::visit([&](auto &rhs) -> EvalConstant::Constant {
            using T2 = std::decay_t<decltype(rhs)>;
            switch (binaryExpr->op)
            {
            case BinaryOp::add: {
                return lhs + rhs;
            }
            case BinaryOp::sub: {
                return lhs - rhs;
            }
            case BinaryOp::mul: {
                return lhs * rhs;
            }
            case BinaryOp::div: {
                return lhs / rhs;
            }
            case BinaryOp::mod: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }
                else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }
                else {
                    return lhs % rhs;
                }
            }
            case BinaryOp::equal: {
                return lhs == rhs;
            }
            case BinaryOp::not_equal: {
                return lhs != rhs;
            }                                                            
            case BinaryOp::less: {
                return lhs < rhs;
            }
            case BinaryOp::less_equal: {
                return lhs <= rhs;
            }
            case BinaryOp::greater: {
                return lhs > rhs;
            }
            case BinaryOp::greater_equal: {
                return lhs >= rhs;
            }  
            case BinaryOp::logical_or: {
                return lhs || rhs;
            } 
            case BinaryOp::logical_and: {
                return lhs && rhs;
            } 
            case BinaryOp::bitwise_or: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else {
                    return lhs | rhs;
                }
            } 
            case BinaryOp::bitwise_xor: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else {
                    return lhs ^ rhs;
                }
            } 
            case BinaryOp::bitwise_and: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else {
                    return lhs & rhs;
                }
            } 
            case BinaryOp::left_shift: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else {
                    return lhs << rhs;
                }
            }
            case BinaryOp::right_shift: {
                if constexpr (!std::is_same_v<int64_t, T1>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else if constexpr (!std::is_same_v<int64_t, T2>) {
                    diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                    return 0;
                }else {
                    return lhs >> rhs;
                }
            }
            case BinaryOp::comma: {
                return rhs;
            }                                                                                                                                   
            default:
                diagEngine.Report(llvm::SMLoc::getFromPointer(binaryExpr->tok.ptr), diag::err_constant_expr);
                return 0;
            }
        }, right);
    }, left);
}

EvalConstant::Constant EvalConstant::VisitUnaryExpr(UnaryExpr *expr) {
    EvalConstant::Constant val = Eval(expr->node.get());
    switch (expr->op)
    {
    case UnaryOp::positive:
        return val;
    case UnaryOp::negative:
        return std::visit([](auto &lhs)->EvalConstant::Constant {
            return -lhs;
        }, val);
    case UnaryOp::logical_not:
        return std::visit([](auto &lhs)->EvalConstant::Constant {
            return !lhs;
        }, val);
    case UnaryOp::bitwise_not:
        return std::visit([&](auto &lhs)->EvalConstant::Constant {
            using T = std::decay_t<decltype(lhs)>;
            if constexpr (!std::is_same_v<int64_t, T>) {
                diagEngine.Report(llvm::SMLoc::getFromPointer(expr->tok.ptr), diag::err_constant_expr);
                return 0;
            }else {
                return ~lhs;
            }
        }, val);              
    default:
        diagEngine.Report(llvm::SMLoc::getFromPointer(expr->tok.ptr), diag::err_constant_expr);
        return 0;
    }
}

/*
 TY_Char,
        TY_UChar,
        TY_Short,
        TY_UShort,
        TY_Int,
        TY_UInt,
        TY_Long,
        TY_ULong,
        TY_LLong,
        TY_ULLong,
        TY_Float,
        TY_Double,
        TY_LDouble,*/
EvalConstant::Constant EvalConstant::VisitCastExpr(CastExpr *expr) {
    EvalConstant::Constant val = Eval(expr->node.get());
    if (expr->targetType) {
        if (!expr->targetType->IsArithType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(expr->tok.ptr), diag::err_constant_expr);
            return 0;
        }
        auto kind = expr->targetType->GetKind();
        switch (kind)
        {
        case CType::TY_Char:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (int8_t)lhs;
            }, val); 
        case CType::TY_UChar:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (uint8_t)lhs;
            }, val);
        case CType::TY_Short:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (int16_t)lhs;
            }, val);
        case CType::TY_UShort:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (uint16_t)lhs;
            }, val);
        case CType::TY_Int:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (int32_t)lhs;
            }, val);
        case CType::TY_UInt:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (uint32_t)lhs;
            }, val);
        case CType::TY_Long:
        case CType::TY_ULong:
        case CType::TY_LLong:
        case CType::TY_ULLong:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (int64_t)lhs;
            }, val);                                                                                            
        case CType::TY_Float:
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (float)lhs;
            }, val); 
        case CType::TY_Double:
        case CType::TY_LDouble:        
            return std::visit([&](auto &lhs)->EvalConstant::Constant {
                return (double)lhs;
            }, val);                    
        default:
            diagEngine.Report(llvm::SMLoc::getFromPointer(expr->tok.ptr), diag::err_constant_expr);
            return 0;
        }
    }
    return val;
}
EvalConstant::Constant EvalConstant::VisitSizeOfExpr(SizeOfExpr *expr) {
    if (expr->type) {
        return expr->type->GetSize() / 8;
    }else {
        return expr->node->ty->GetSize() / 8;
    }
}

EvalConstant::Constant EvalConstant::VisitThreeExpr(ThreeExpr *expr) {
    EvalConstant::Constant cond = Eval(expr->cond.get());
    EvalConstant::Constant then = Eval(expr->then.get());
    EvalConstant::Constant els = Eval(expr->els.get());

    return std::visit([&](auto &lhs)->EvalConstant::Constant {
        if (lhs) {
            return then;
        }else {
            return els;
        }
    }, cond);   
}