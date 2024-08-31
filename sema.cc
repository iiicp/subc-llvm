#include "sema.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Casting.h"

std::shared_ptr<AstNode> Sema::SemaVariableDeclNode(Token tok, std::shared_ptr<CType> ty, bool isGlobal) {
    // 1. 检测是否出现重定义
    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindObjSymbolInCurEnv(text);
    if (symbol && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, text);
    }
    if (GetMode() == Mode::Normal) {
        /// 2. 添加到符号表
        scope.AddObjSymbol(ty, text);
    }

    /// 3. 返回结点
    auto decl = std::make_shared<VariableDecl>();
    decl->tok = tok;
    decl->ty = ty;
    decl->isLValue = true;
    decl->isGlobal = isGlobal;
    return decl;
}

std::shared_ptr<AstNode> Sema::SemaVariableAccessNode(Token tok)  {

    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindObjSymbol(text);
    if (symbol == nullptr && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_undefined, text);
    }

    auto expr = std::make_shared<VariableAccessExpr>();
    expr->tok = tok;
    expr->ty = symbol->GetTy();
    expr->isLValue = true;
    return expr;
}

std::shared_ptr<AstNode> Sema::SemaBinaryExprNode( std::shared_ptr<AstNode> left,std::shared_ptr<AstNode> right, BinaryOp op, Token tok) {
    auto binaryExpr = std::make_shared<BinaryExpr>();
    binaryExpr->tok = tok;
    binaryExpr->op = op;
    binaryExpr->left = left;
    binaryExpr->right = right;
    binaryExpr->ty = left->ty;

    CType::Kind leftKind = left->ty->GetKind();
    CType::Kind rightKind = right->ty->GetKind();

    switch (op)
    {
    case BinaryOp::add: {
        if (!left->ty->IsArithType() && leftKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (!right->ty->IsArithType() && rightKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (leftKind == CType::TY_Point && rightKind == CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (leftKind == CType::TY_Point && !right->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (rightKind == CType::TY_Point && !left->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (rightKind == CType::TY_Point) {
            binaryExpr->ty = right->ty;
            auto t = binaryExpr->left;
            binaryExpr->left = binaryExpr->right;
            binaryExpr->right = t;
        }
        break;
    }
    case BinaryOp::sub: {
        if (!left->ty->IsArithType() && leftKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (!right->ty->IsArithType() && rightKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (leftKind == CType::TY_Point && leftKind == CType::TY_Point) {
            binaryExpr->ty = CType::LongType;
        }else if (leftKind == CType::TY_Point && !right->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (rightKind == CType::TY_Point && !left->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (rightKind == CType::TY_Point) {
            binaryExpr->ty = right->ty;
            auto t = binaryExpr->left;
            binaryExpr->left = binaryExpr->right;
            binaryExpr->right = t;
        }
        break;
    }
    case BinaryOp::add_assign:
    case BinaryOp::sub_assign: {
        if (!left->ty->IsArithType() && leftKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (!right->ty->IsArithType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (leftKind == CType::TY_Point && !right->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }
        break;
    }
    case BinaryOp::mul: 
    case BinaryOp::mul_assign:
    case BinaryOp::div: 
    case BinaryOp::div_assign: {
        if (!left->ty->IsArithType() && !right->ty->IsArithType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }
        break;
    }
    case BinaryOp::mod: 
    case BinaryOp::mod_assign: 
    case BinaryOp::bitwise_or: 
    case BinaryOp::bitwise_or_assign:
    case BinaryOp::bitwise_and: 
    case BinaryOp::bitwise_and_assign:
    case BinaryOp::bitwise_xor:
    case BinaryOp::bitwise_xor_assign: 
    case BinaryOp::left_shift:
    case BinaryOp::left_shift_assign:
    case BinaryOp::right_shift:
    case BinaryOp::right_shift_assign:{
        if (!left->ty->IsIntegerType() && !right->ty->IsIntegerType()) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }
        break;
    }
    case BinaryOp::equal: 
    case BinaryOp::not_equal:
    case BinaryOp::less:
    case BinaryOp::less_equal:
    case BinaryOp::greater:
    case BinaryOp::greater_equal:
    case BinaryOp::logical_or:
    case BinaryOp::logical_and:{
        if (!left->ty->IsArithType() && leftKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }else if (!right->ty->IsArithType() && rightKind != CType::TY_Point) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_binary_expr_type);
        }
        break;
    }         
    default:
        break;
    }

    return binaryExpr;
}

std::shared_ptr<AstNode> Sema::SemaUnaryExprNode( std::shared_ptr<AstNode> unary, UnaryOp op, Token tok) {
    auto node = std::make_shared<UnaryExpr>();
    node->op = op;
    node->node = unary;

    switch (op)
    {
    case UnaryOp::positive:
    case UnaryOp::negative:
    case UnaryOp::logical_not:
    case UnaryOp::bitwise_not:
    {
        if (unary->ty->GetKind() != CType::TY_Int && (GetMode() == Mode::Normal)) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_ype, "int type");
        }
        node->ty = unary->ty;
        break;
    }
    case UnaryOp::addr: {
        /// &a; 
        // if (!unary->isLValue) {
        //     diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_lvalue);
        // }
        node->ty = std::make_shared<CPointType>(unary->ty);
        break;
    }
    case UnaryOp::deref: {
        /// *a;
        /// 语义判断 must be pointer
        if (unary->ty->GetKind() != CType::TY_Point && (GetMode() == Mode::Normal)) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_ype, "pointer type");
        }
        // if (!unary->isLValue) {
        //     diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_lvalue);
        // }
        CPointType *pty = llvm::dyn_cast<CPointType>(unary->ty.get());
        node->ty = pty->GetBaseType();
        node->isLValue = true;
        break;   
    }
    case UnaryOp::dec:
    case UnaryOp::inc: {
        if (!unary->isLValue) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_lvalue);
        }
        node->ty = unary->ty;
        break;          
    }                       
    default:
        break;
    }

    return node;
}

std::shared_ptr<AstNode> Sema::SemaCastExprNode( std::shared_ptr<CType> targetType, std::shared_ptr<AstNode> node, Token tok) {
    auto ret = std::make_shared<CastExpr>();
    ret->ty = targetType;
    ret->targetType = targetType;
    ret->node = node;
    ret->tok = tok;
    return ret;
}

std::shared_ptr<AstNode> Sema::SemaThreeExprNode( std::shared_ptr<AstNode> cond,std::shared_ptr<AstNode> then, std::shared_ptr<AstNode> els, Token tok) {
    auto node = std::make_shared<ThreeExpr>();
    node->cond = cond;
    node->then = then;
    node->els = els;
    if (then->ty->GetKind() != els->ty->GetKind() && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_same_type);
    }
    node->ty = then->ty;
    return node;
}

// sizeof a;
std::shared_ptr<AstNode> Sema::SemaSizeofExprNode( std::shared_ptr<AstNode> unary,std::shared_ptr<CType> ty) {
    auto node = std::make_shared<SizeOfExpr>();
    node->type = ty;
    node->node = unary;
    node->ty = CType::IntType;
    return node;
}

std::shared_ptr<AstNode> Sema::SemaPostIncExprNode(std::shared_ptr<AstNode> left, Token tok) {
    if (!left->isLValue && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_lvalue);
    }
    auto node = std::make_shared<PostIncExpr>();
    node->left = left;
    node->ty = left->ty;
    return node;
}

/// a--
std::shared_ptr<AstNode> Sema::SemaPostDecExprNode( std::shared_ptr<AstNode> left, Token tok) {
    if (!left->isLValue && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_lvalue);
    }
    auto node = std::make_shared<PostDecExpr>();
    node->left = left;
    node->ty = left->ty;
    return node;
}
/// a[1]; -> *(a + offset(1 * elementSize));
std::shared_ptr<AstNode> Sema::SemaPostSubscriptNode(std::shared_ptr<AstNode> left, std::shared_ptr<AstNode> node, Token tok) {
    if (left->ty->GetKind() != CType::TY_Array && left->ty->GetKind() != CType::TY_Point && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_expected_ype, "array or point");
    }
    auto postSubScriptNode = std::make_shared<PostSubscript>();
    postSubScriptNode->left = left;
    postSubScriptNode->node = node;
    if (left->ty->GetKind() == CType::TY_Array) {
        CArrayType *arrType = llvm::dyn_cast<CArrayType>(left->ty.get());
        postSubScriptNode->ty = arrType->GetElementType();
    }else if (left->ty->GetKind() == CType::TY_Point) {
        CPointType *pointType = llvm::dyn_cast<CPointType>(left->ty.get());
        postSubScriptNode->ty = pointType->GetBaseType();
    }
    return postSubScriptNode;
}

std::shared_ptr<AstNode> Sema::SemaPostMemberDotNode(std::shared_ptr<AstNode> left, Token iden, Token dotTok) {
    if (left->ty->GetKind() != CType::TY_Record && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(dotTok.ptr), diag::err_expected_ype, "struct or union type");
    }

    CRecordType *recordType = llvm::dyn_cast<CRecordType>(left->ty.get());
    const auto &members = recordType->GetMembers();

    bool found = false;
    Member curMember;
    for (const auto &member : members) {
        if (member.name == llvm::StringRef(iden.ptr, iden.len)) {
            found = true;
            curMember = member;
            break;
        }
    }
    if (!found) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(iden.ptr), diag::err_miss, "struct or union miss field");
    }

    
    auto node = std::make_shared<PostMemberDotExpr>();
    node->tok = dotTok;
    node->ty = curMember.ty;
    node->left = left;
    node->member = curMember;
    return node;
}

std::shared_ptr<AstNode> Sema::SemaPostMemberArrowNode(std::shared_ptr<AstNode> left, Token iden, Token arrowTok) {
    if (left->ty->GetKind() != CType::TY_Point) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(arrowTok.ptr), diag::err_expected_ype, "pointer type");
    }

    CPointType *pRecordType = llvm::dyn_cast<CPointType>(left->ty.get());
    if (pRecordType->GetBaseType()->GetKind() != CType::TY_Record) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(arrowTok.ptr), diag::err_expected_ype, "pointer to struct or union type");
    }

    CRecordType *recordType = llvm::dyn_cast<CRecordType>(pRecordType->GetBaseType().get());
    const auto &members = recordType->GetMembers();

    bool found = false;
    Member curMember;
    for (const auto &member : members) {
        if (member.name == llvm::StringRef(iden.ptr, iden.len)) {
            found = true;
            curMember = member;
            break;
        }
    }
    if (!found) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(iden.ptr), diag::err_miss, "struct or union miss field");
    }

    
    auto node = std::make_shared<PostMemberArrowExpr>();
    node->tok = arrowTok;
    node->ty = curMember.ty;
    node->left = left;
    node->member = curMember;
    return node;
}

std::shared_ptr<AstNode> Sema::SemaNumberExprNode(Token tok,int val,std::shared_ptr<CType> ty) {
    auto expr = std::make_shared<NumberExpr>();
    expr->tok = tok;
    expr->ty = ty;
    expr->value.v = val;
    return expr;
}

std::shared_ptr<AstNode> Sema::SemaNumberExprNode(Token tok, std::shared_ptr<CType> ty) {
    auto expr = std::make_shared<NumberExpr>();
    expr->tok = tok;
    expr->ty = ty;
    if (ty->IsIntegerType()) {
        expr->value.v = tok.value.v;
    }else {
        expr->value.d = tok.value.d;
    }
    return expr;  
}

std::shared_ptr<AstNode> Sema::SemaStringExprNode(Token tok, std::string val, std::shared_ptr<CType> ty) {
    auto expr = std::make_shared<StringExpr>();
    expr->tok = tok;
    expr->ty = ty;
    expr->value = val;
    return expr;
}

std::shared_ptr<VariableDecl::InitValue> Sema::SemaDeclInitValue(std::shared_ptr<CType> declType, std::shared_ptr<AstNode> value, std::vector<int> &offsetList, Token tok)
 {
    // if (declType->GetKind() != value->ty->GetKind() && (GetMode() == Mode::Normal)) {
    //     diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_miss, "same type");
    // }
    auto initValue = std::make_shared<VariableDecl::InitValue>();
    initValue->declType = declType;
    initValue->value = value;
    initValue->offsetList = offsetList;
    return initValue;
 }

std::shared_ptr<AstNode> Sema::SemaIfStmtNode(std::shared_ptr<AstNode> condNode, std::shared_ptr<AstNode> thenNode, std::shared_ptr<AstNode> elseNode) {
    auto node = std::make_shared<IfStmt>();
    node->condNode = condNode;
    node->thenNode = thenNode;
    node->elseNode = elseNode;
    return node;
}

std::shared_ptr<CType> Sema::SemaTagAccess(Token tok) {
    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindTagSymbol(text);
    if (symbol) {
        return symbol->GetTy();;
    }
    return nullptr;
}

std::shared_ptr<CType> Sema::SemaTagDecl(Token tok, const std::vector<Member> &members, TagKind tagKind) {
    // 1. 检测是否出现重定义
    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindTagSymbolInCurEnv(text);
    if (symbol) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, text);
    }
    auto recordTy = std::make_shared<CRecordType>(text, members, tagKind);
    if (GetMode() == Mode::Normal) {
        /// 2. 添加到符号表
        scope.AddTagSymbol(recordTy, text);
    }
    return recordTy;
}

std::shared_ptr<CType> Sema::SemaTagDecl(Token tok, std::shared_ptr<CType> type) {
    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindTagSymbolInCurEnv(text);
    if (symbol) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, text);
    }
    if (GetMode() == Mode::Normal) {
        /// 2. 添加到符号表
        scope.AddTagSymbol(type, text);
    }
    return type;
}

std::shared_ptr<CType> Sema::SemaAnonyTagDecl(const std::vector<Member> &members, TagKind tagKind) {
    llvm::StringRef text = CType::GenAnonyRecordName(tagKind);
    auto recordTy = std::make_shared<CRecordType>(text, members, tagKind);
    if (GetMode() == Mode::Normal) {
        /// 2. 添加到符号表
        scope.AddTagSymbol(recordTy, text);
    }
    return recordTy;
}

std::shared_ptr<AstNode> Sema::SemaFuncDecl(Token tok, std::shared_ptr<CType> type, std::shared_ptr<AstNode> blockStmt) {
    CFuncType *funTy = llvm::dyn_cast<CFuncType>(type.get());
    funTy->hasBody = (blockStmt) ? true : false;

     // 1. 检测是否出现重定义
    llvm::StringRef text(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindObjSymbolInCurEnv(text);
    if (symbol) {
        auto symTy = symbol->GetTy();
        if (symTy->GetKind() != CType::TY_Func && (GetMode() == Mode::Normal)) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, text);
        }
        CFuncType *symbolFunTy = llvm::dyn_cast<CFuncType>(symTy.get());
        if (symbolFunTy->hasBody && funTy->hasBody && (GetMode() == Mode::Normal)) {
            diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, text);
        }
    }

    if ((symbol == nullptr || funTy->hasBody)  && (GetMode() == Mode::Normal)) {
        /// 2. 添加到符号表
        scope.AddObjSymbol(type, text);
    }

    auto funcDecl = std::make_shared<FuncDecl>();
    funcDecl->ty = type;
    funcDecl->blockStmt = blockStmt;
    funcDecl->tok = tok;
    return funcDecl;
}

std::shared_ptr<AstNode> Sema::SemaFuncCall(std::shared_ptr<AstNode> left, const std::vector<std::shared_ptr<AstNode>> &args) {
    Token iden = left->tok;
    CFuncType *cFuncTyPtr = nullptr;
    std::shared_ptr<CType> funcTy = nullptr;
    if (left->ty->GetKind() == CType::TY_Point) {
        CPointType *pty = llvm::dyn_cast<CPointType>(left->ty.get());
        if (pty->GetBaseType()->GetKind() == CType::TY_Func) {
            cFuncTyPtr = llvm::dyn_cast<CFuncType>(pty->GetBaseType().get());
            funcTy = pty->GetBaseType();
        }else {
            if (GetMode() == Mode::Normal) {
                diagEngine.Report(llvm::SMLoc::getFromPointer(iden.ptr), diag::err_expected, "functype");
            }
        }
    }
    else if (left->ty->GetKind() != CType::TY_Func && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(iden.ptr), diag::err_expected, "functype");
    }
    
    if (!cFuncTyPtr) {
        cFuncTyPtr = llvm::dyn_cast<CFuncType>(left->ty.get());
    }
    
    if ((cFuncTyPtr->GetParams().size() != args.size()) && !cFuncTyPtr->IsVarArg() && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(iden.ptr), diag::err_miss, "arg count not match");
    }

    auto funcCall = std::make_shared<PostFuncCall>();
    funcCall->ty = cFuncTyPtr->GetRetType();
    if (funcTy) {
        left->ty = funcTy;
    }
    funcCall->left = left;
    funcCall->args = args;
    funcCall->tok = left->tok;
    return funcCall;
}

void Sema::SemaTypedefDecl(std::shared_ptr<CType> type, Token tok) {
    llvm::StringRef name(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindObjSymbolInCurEnv(name);
    if (symbol && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_redefined, name);
    }
    if (GetMode() == Mode::Normal) {
        /// 2. 添加到符号表
        scope.AddTypedefSymbol(type, name);
    }
}

std::shared_ptr<CType> Sema::SemaTypedefAccess(Token tok) {
    llvm::StringRef name(tok.ptr, tok.len);
    std::shared_ptr<Symbol> symbol = scope.FindObjSymbol(name);
    if (symbol == nullptr && (GetMode() == Mode::Normal)) {
        diagEngine.Report(llvm::SMLoc::getFromPointer(tok.ptr), diag::err_undefined, name);
    }
    if (symbol && symbol->GetKind() == SymbolKind::ktypedef) {
        return symbol->GetTy();
    }
    return nullptr;
}

void Sema::EnterScope() {
    scope.EnterScope();
}

void Sema::ExitScope() {
    scope.ExitScope();
}

void Sema::SetMode(Mode mode) {
    modeStack.push(mode);
}

void Sema::UnSetMode() {
    modeStack.pop();
}

Sema::Mode Sema::GetMode() {
    return modeStack.top();
}
