#include "print_visitor.h"

PrintVisitor::PrintVisitor(std::shared_ptr<Program> program, llvm::raw_ostream *out) {
    this->out = out;
    VisitProgram(program.get());
}

llvm::Value * PrintVisitor::VisitProgram(Program *p) {
    for (const auto &decl : p->externalDecls) {
        decl->Accept(this);
    }
    return nullptr;
}

llvm::Value * PrintVisitor::VisitBlockStmt(BlockStmt *p) {
    *out << "{";
    for (const auto &stmt : p->nodeVec) {
        stmt->Accept(this);
        *out << ";";
    }
    *out << "}";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitDeclStmt(DeclStmt *p) {
    int i = 0, size = p->nodeVec.size();
    for (const auto &node : p->nodeVec) {
        node->Accept(this);
        if (i != size-1) {
            *out << ";";
        }
        ++i;
    }
    return nullptr;
}

llvm::Value * PrintVisitor::VisitIfStmt(IfStmt *p) {
    *out << "if(";
    p->condNode->Accept(this);
    *out << ")";
    p->thenNode->Accept(this);
    if (p->elseNode) {
        *out << "else";
        p->elseNode->Accept(this);
    }
    return nullptr;
}

llvm::Value * PrintVisitor::VisitForStmt(ForStmt *p) {
    *out << "for(";
    if (p->initNode) {
        p->initNode->Accept(this);
    }
    *out << ";";
    if (p->condNode) {
        p->condNode->Accept(this);
    }
    *out << ";";
    if (p->incNode) {
        p->incNode->Accept(this);
    }
    *out << ")";

    if (p->bodyNode) {
        p->bodyNode->Accept(this);
    }

    return nullptr;
}

llvm::Value * PrintVisitor::VisitContinueStmt(ContinueStmt *p) {
    *out << "continue";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitReturnStmt(ReturnStmt *p) {
    *out << "return ";
    if (p->expr) {
        p->expr->Accept(this);
    }

    return nullptr;
}

llvm::Value * PrintVisitor::VisitBreakStmt(BreakStmt *p) {
    *out << "break";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitSwitchStmt(SwitchStmt *p) {
    *out << "switch(";
    p->expr->Accept(this);
    *out << ")";
    p->stmt->Accept(this);

    return nullptr;
}

llvm::Value * PrintVisitor::VisitCaseStmt(CaseStmt *p) {
    *out << "case ";
    p->expr->Accept(this);
    *out << ":";
    p->stmt->Accept(this);

    return nullptr;
}

llvm::Value * PrintVisitor::VisitDefaultStmt(DefaultStmt *p) {
    *out << "default:";
    p->stmt->Accept(this);

    return nullptr;
}

llvm::Value * PrintVisitor::VisitDoWhileStmt(DoWhileStmt *p) {
    *out << "do ";
    p->stmt->Accept(this);
    *out << "while (";
    p->expr->Accept(this);
    *out << ");";

    return nullptr;
}

llvm::Value * PrintVisitor::VisitVariableDecl(VariableDecl *decl) {
    decl->ty->Accept(this);
    llvm::StringRef text(decl->tok.ptr, decl->tok.len);
    *out << text;
    if (decl->initValues.size() > 0) {
        *out << "=";
    }
    int i = 0, size = decl->initValues.size();
    for (const auto &initValue : decl->initValues) {
        initValue->value->Accept(this);
        if (i < size-1) {
            *out << ",";
        }
        ++i;
    }
    return nullptr;
}

llvm::Value * PrintVisitor::VisitFuncDecl(FuncDecl *decl) {
    decl->ty->Accept(this);
    if (decl->blockStmt) {
        decl->blockStmt->Accept(this);
    }else {
        *out << ";";
    }
    return nullptr;
}

llvm::Value * PrintVisitor::VisitBinaryExpr(BinaryExpr *binaryExpr) {
    binaryExpr->left->Accept(this);

    switch (binaryExpr->op)
    {
    case BinaryOp::add: {
        *out << "+";
        break;
    }
    case BinaryOp::sub:{
        *out << "-";
        break;
    }
    case BinaryOp::mul:{
        *out << "*";
        break;
    }
    case BinaryOp::div:{
        *out << "/";
        break;
    }     
    case BinaryOp::equal:{
        *out << "==";
        break;
    } 
    case BinaryOp::not_equal:{
        *out << "!=";
        break;
    } 
    case BinaryOp::less:{
        *out << "<";
        break;
    } 
    case BinaryOp::less_equal:{
        *out << "<=";
        break;
    } 
    case BinaryOp::greater:{
        *out << ">";
        break;
    } 
    case BinaryOp::greater_equal:{
        *out << ">=";
        break;
    }  
    case BinaryOp::mod:{
        *out << "%";
        break;
    } 
    case BinaryOp::logical_or:{
        *out << "||";
        break;
    } 
    case BinaryOp::logical_and:{
        *out << "&&";
        break;
    } 
    case BinaryOp::bitwise_and:{
        *out << "&";
        break;
    } 
    case BinaryOp::bitwise_or:{
        *out << "|";
        break;
    } 
    case BinaryOp::bitwise_xor:{
        *out << "^";
        break;
    }   
    case BinaryOp::left_shift:{
        *out << "<<";
        break;
    }  
    case BinaryOp::right_shift:{
        *out << ">>";
        break;
    } 
    case BinaryOp::comma: {
        *out << ",";
        break;
    }  
    case BinaryOp::assign: {
        *out << "=";
        break;
    }  
    case BinaryOp::add_assign: {
        *out << "+=";
        break;
    }  
    case BinaryOp::sub_assign: {
        *out << "-=";
        break;
    }  
    case BinaryOp::mul_assign: {
        *out << "*=";
        break;
    }  
    case BinaryOp::div_assign: {
        *out << "/=";
        break;
    }  
    case BinaryOp::mod_assign: {
        *out << "%=";
        break;
    }  
    case BinaryOp::bitwise_and_assign: {
        *out << "&=";
        break;
    }  
    case BinaryOp::bitwise_or_assign: {
        *out << "|=";
        break;
    }  
    case BinaryOp::bitwise_xor_assign: {
        *out << "^=";
        break;
    }  
    case BinaryOp::left_shift_assign: {
        *out << "<<=";
        break;
    }  
    case BinaryOp::right_shift_assign: {
        *out << ">>=";
        break;
    }  
    default:
        break;
    }

    binaryExpr->right->Accept(this);

    return nullptr;
}

llvm::Value * PrintVisitor::VisitNumberExpr(NumberExpr *expr) {

    *out << expr->value;

    return nullptr;
}

llvm::Value * PrintVisitor::VisitStringExpr(StringExpr *expr) {
    *out << llvm::StringRef(expr->tok.ptr, expr->tok.len);
    return nullptr;
}

llvm::Value * PrintVisitor::VisitVariableAccessExpr(VariableAccessExpr *expr) {
    *out << llvm::StringRef(expr->tok.ptr, expr->tok.len);
    return nullptr;
}

llvm::Value * PrintVisitor::VisitUnaryExpr(UnaryExpr *expr) {
    switch (expr->op)
    {
    case UnaryOp::positive:
        *out << "+";
        break;
    case UnaryOp::negative:
        *out << "-";
        break;
    case UnaryOp::deref:
        *out << "*";
        break;
    case UnaryOp::addr:
        *out << "&";
        break;
    case UnaryOp::logical_not:
        *out << "!";
        break;
    case UnaryOp::bitwise_not:
        *out << "~";
        break;
    case UnaryOp::inc:
        *out << "++";
        break;
    case UnaryOp::dec:
        *out << "--";
        break;                                                         
    default:
        break;
    }

    expr->node->Accept(this);

    return nullptr;
}
llvm::Value * PrintVisitor::VisitSizeOfExpr(SizeOfExpr *expr) {
    *out << "sizeof ";
    if (expr->type) {
        *out << "(";
        expr->type->Accept(this);
        *out << ")";
    }else {
        expr->node->Accept(this);
    }

    return nullptr;
}
llvm::Value * PrintVisitor::VisitPostIncExpr(PostIncExpr *expr) {
    expr->left->Accept(this);
    *out << "++";
    return nullptr;
}
llvm::Value * PrintVisitor::VisitPostDecExpr(PostDecExpr *expr) {
    expr->left->Accept(this);
    *out << "--";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitPostSubscript(PostSubscript *expr) {
    expr->left->Accept(this);
    *out<<"[";
    expr->node->Accept(this);
    *out<<"]";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitPostMemberDotExpr(PostMemberDotExpr *expr) {
    expr->left->Accept(this);
    *out<<".";
    *out << expr->member.name;
    return nullptr;
}

llvm::Value * PrintVisitor::VisitPostMemberArrowExpr(PostMemberArrowExpr *expr) {
    expr->left->Accept(this);
    *out<<"->";
    *out << expr->member.name;
    return nullptr;
}

llvm::Value * PrintVisitor::VisitPostFuncCall(PostFuncCall *expr) {
    expr->left->Accept(this);
    *out << "(";
    int i = 0, size = expr->args.size();
    for (const auto &arg : expr->args) {
        arg->Accept(this);
        if (i < size - 1) {
            *out << ",";
        }
    }
    *out << ")";
    return nullptr;
}

llvm::Value * PrintVisitor::VisitThreeExpr(ThreeExpr *expr) {
    expr->cond->Accept(this);
    *out<<"?";
    expr->then->Accept(this);
    *out<<":";
    expr->els->Accept(this);
    
    return nullptr;
}

llvm::Type * PrintVisitor::VisitPrimaryType(CPrimaryType *ty) {
    if (ty->GetKind() == CType::TY_Int) {
        *out << "int ";
    }else if (ty->GetKind() == CType::TY_Void) {
        *out << "void ";
    }

    return nullptr;
}

llvm::Type * PrintVisitor::VisitPointType(CPointType *ty) {
    /// int **p
    ty->GetBaseType()->Accept(this);
    *out << "*";

    return nullptr;
}

/// a[3][4];
/// [3][4]int
llvm::Type * PrintVisitor::VisitArrayType(CArrayType *ty) {
    *out<<"["<<ty->GetElementCount()<<"]";
    ty->GetElementType()->Accept(this);
    return nullptr;
}

llvm::Type * PrintVisitor::VisitRecordType(CRecordType *ty) {
    TagKind tagKind = ty->GetTagKind();
    if (tagKind == TagKind::kStruct) {
        *out << "struct ";
    }else {
        *out << "union ";
    }
    *out << ty->GetName() << "{";
    for (const auto &m : ty->GetMembers()) {
        m.ty->Accept(this);
        *out << m.name << ";";
    }
    *out << "} ";

    return nullptr;
}

llvm::Type * PrintVisitor::VisitFuncType(CFuncType *ty) {
    ty->GetRetType()->Accept(this);
    *out << ty->GetName() << "(";
    int i = 0, size = ty->GetParams().size();
    for (const auto &p : ty->GetParams()) {
        p.type->Accept(this);
        *out << p.name;
        if (i < size - 1) {
            *out << ",";
        }
    }
    if (ty->IsVarArg()) {
        *out << ",...";
    }
    *out << ")";
    return nullptr;
}