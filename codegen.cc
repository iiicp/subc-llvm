#include "codegen.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include <cassert>

using namespace llvm;

/// ir 常量折叠 
/// 编译的版本是 Release and Debug Symbol
/*
define i32 @main() {
entry:
  %0 = call i32 (ptr, ...) @printf(ptr @0, i32 4)
  %1 = call i32 (ptr, ...) @printf(ptr @1, i32 10)
  %2 = call i32 (ptr, ...) @printf(ptr @2, i32 -1)
  ret i32 0
}
*/
llvm::Value * CodeGen::VisitProgram(Program *p) {
    for (const auto &decl : p->externalDecls) {
        decl->Accept(this);
    }
    return nullptr;
}

llvm::Value * CodeGen::VisitBinaryExpr(BinaryExpr *binaryExpr) {
    llvm::Value *left = nullptr;
    llvm::Value *right = nullptr;
    if (binaryExpr->op != BinaryOp::logical_or && binaryExpr->op != BinaryOp::logical_and) {
        left = binaryExpr->left->Accept(this);
        right = binaryExpr->right->Accept(this);
    }
    switch (binaryExpr->op)
    {
    case BinaryOp::add: {
        BinaryArithCastValue(left, right);
        if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
            assert(0 && "not support pointer add pointer");
        }
        if (left->getType()->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(left->getType(), left, {right});
            return newVal;
        }else if (right->getType()->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(right->getType(), right, {left});
            return newVal;
        }else if (left->getType()->isIntegerTy()) {
            return irBuilder.CreateNSWAdd(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            return irBuilder.CreateFAdd(left, right);
        }else {
            assert(0 && "type support add");
        }
    }
    case BinaryOp::sub:{
        BinaryArithCastValue(left, right);
        if (left->getType()->isPointerTy() && right->getType()->isPointerTy()) {
            llvm::Value *t = irBuilder.CreateNSWSub(left, right);
            /// 减法之后的结果为整型64位
            AssignCastValue(t, irBuilder.getInt64Ty());
            return t;
        }else if (left->getType()->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(left->getType(), left, {irBuilder.CreateNeg(right)});
            return newVal;
        }else if (right->getType()->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(right->getType(), right, {irBuilder.CreateNeg(left)});
            return newVal;
        }else if (left->getType()->isIntegerTy()){
            return irBuilder.CreateNSWSub(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            return irBuilder.CreateFSub(left, right);
        }else {
            assert(0 && "type support sub");
        }
    }
    case BinaryOp::mul:{
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntegerTy()) {
            return irBuilder.CreateNSWMul(left, right);
        }else {
            return irBuilder.CreateFMul(left, right);
        }
    }
    case BinaryOp::div:{
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntegerTy()) {
            return irBuilder.CreateSDiv(left, right);
        }else {
            return irBuilder.CreateFDiv(left, right);
        }
    }
    case BinaryOp::mod: {
        BinaryArithCastValue(left, right);
        return irBuilder.CreateSRem(left, right);
    }
    case BinaryOp::bitwise_and:{    
        BinaryArithCastValue(left, right);  
        return irBuilder.CreateAnd(left, right);
    }
    case BinaryOp::bitwise_or:{    
        BinaryArithCastValue(left, right);  
        return irBuilder.CreateOr(left, right);
    }
    case BinaryOp::bitwise_xor:{     
        BinaryArithCastValue(left, right);
        return irBuilder.CreateXor(left, right);
    }   
    case BinaryOp::left_shift:{    
        BinaryArithCastValue(left, right);
        return irBuilder.CreateShl(left, right);
    }
    case BinaryOp::right_shift:{  
        BinaryArithCastValue(left, right);
        return irBuilder.CreateAShr(left, right);
    }        
    case BinaryOp::equal: {      
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {
            left = irBuilder.CreateICmpEQ(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpUEQ(left, right);
        }
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::not_equal:{   
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {   
            left = irBuilder.CreateICmpNE(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpUNE(left, right);
        }
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::less:{     
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {
            left = irBuilder.CreateICmpSLT(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpULT(left, right);
        }
        
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::less_equal:{ 
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {
            left = irBuilder.CreateICmpSLE(left, right);    
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpULE(left, right);
        }    
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }      
    case BinaryOp::greater:{    
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {  
            left = irBuilder.CreateICmpSGT(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpUGT(left, right);
        }
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::greater_equal:{ 
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntOrPtrTy()) {
            left = irBuilder.CreateICmpSGE(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFCmpUGE(left, right);
        }
        return irBuilder.CreateIntCast(left, irBuilder.getInt32Ty(), true);
    }  
    case BinaryOp::logical_and:{
        /// A && B

        llvm::BasicBlock *nextBB = llvm::BasicBlock::Create(context, "nextBB");
        llvm::BasicBlock *falseBB = llvm::BasicBlock::Create(context, "falseBB");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "mergeBB");

        llvm::Value *left = binaryExpr->left->Accept(this);
        left = ConvertToBoolVal(left);
        irBuilder.CreateCondBr(left, nextBB, falseBB);

        nextBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(nextBB);
        llvm::Value *right = binaryExpr->right->Accept(this);
        right = ConvertToBoolVal(right);
        /// 32位 0 或着 1
        right = irBuilder.CreateZExt(right, irBuilder.getInt32Ty());
        irBuilder.CreateBr(mergeBB);

        /// right 这个值，所在的基本块，并不一定是 之前的nextBB了.
        /// 原因是：binaryExpr->right->Accept(this) 内部会生成新的基本块

        /// 拿到当前插入的block, 建立一个值和基本块的关系 {right, nextBB}
        auto *nextLastBB = irBuilder.GetInsertBlock();
        
        falseBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(falseBB);
        irBuilder.CreateBr(mergeBB);

        mergeBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(mergeBB);
        llvm::PHINode *phi = irBuilder.CreatePHI(irBuilder.getInt32Ty(), 2);
        phi->addIncoming(right, nextLastBB);
        phi->addIncoming(irBuilder.getInt32(0), falseBB);

        return phi;
    }  
    case BinaryOp::logical_or: {
        /// A || B && C

        llvm::BasicBlock *nextBB = llvm::BasicBlock::Create(context, "nextBB");
        llvm::BasicBlock *trueBB = llvm::BasicBlock::Create(context, "trueBB");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "mergeBB");

        llvm::Value *left = binaryExpr->left->Accept(this);
        left = ConvertToBoolVal(left);
        irBuilder.CreateCondBr(left, trueBB, nextBB);

        nextBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(nextBB);
        /// 右子树内部也生成了基本块
        llvm::Value *right = binaryExpr->right->Accept(this);
        right = ConvertToBoolVal(right);
        /// 32位 0 或着 1
        right = irBuilder.CreateZExt(right, irBuilder.getInt32Ty());
        irBuilder.CreateBr(mergeBB);
        /// right 这个值，所在的基本块，并不一定是 之前的nextBB了.
        /// 原因是：binaryExpr->right->Accept(this) 内部会生成新的基本块

        /// 拿到当前插入的block, 建立一个值和基本块的关系 {right, nextBB}
        auto *nextLastBB = irBuilder.GetInsertBlock();

        trueBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(trueBB);
        irBuilder.CreateBr(mergeBB);

        mergeBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(mergeBB);
        llvm::PHINode *phi = irBuilder.CreatePHI(irBuilder.getInt32Ty(), 2);
        phi->addIncoming(right, nextLastBB);
        phi->addIncoming(irBuilder.getInt32(1), trueBB);

        return phi;
    }
    case BinaryOp::assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        AssignCastValue(right, left->getType());
        irBuilder.CreateStore(right, load->getPointerOperand());
        return right;
    }
    case BinaryOp::add_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        if (left->getType()->isPointerTy()) {
             llvm::Value *newVal = irBuilder.CreateInBoundsGEP(left->getType(), left, {right});
             irBuilder.CreateStore(newVal, load->getPointerOperand());
             return newVal;
        }
        else if (left->getType()->isFloatingPointTy()) {
            llvm::Value *tmp = irBuilder.CreateFAdd(left, right);
            AssignCastValue(tmp, left->getType());
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }
        else if (left->getType()->isIntegerTy()){
            /// a+=3; => a = a + 3;
            llvm::Value *tmp = irBuilder.CreateAdd(left, right);
            AssignCastValue(tmp, left->getType());
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }else {
            assert(0 && "type support add assign");
        }
    }
    case BinaryOp::sub_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        if (left->getType()->isPointerTy()) {
             llvm::Value *newVal = irBuilder.CreateInBoundsGEP(left->getType(), left, {irBuilder.CreateNeg(right)});
             irBuilder.CreateStore(newVal, load->getPointerOperand());
             return newVal;
        }
        else if (left->getType()->isFloatingPointTy()) {
            llvm::Value *tmp = irBuilder.CreateFSub(left, right);
            AssignCastValue(tmp, left->getType());
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }
        else if (left->getType()->isIntegerTy()){
            llvm::Value *tmp = irBuilder.CreateSub(left, right);
            AssignCastValue(tmp, left->getType());
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }else {
            assert(0 && "type support sub assign");
        }
    }
    case BinaryOp::mul_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntegerTy()) {
            left = irBuilder.CreateMul(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFMul(left, right);
        }
        AssignCastValue(left, left->getType());
        irBuilder.CreateStore(left, load->getPointerOperand());
        return left;
    }
    case BinaryOp::div_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        if (left->getType()->isIntegerTy()) {
            left = irBuilder.CreateSDiv(left, right);
        }else if (left->getType()->isFloatingPointTy()) {
            left = irBuilder.CreateFDiv(left, right);
        }
        AssignCastValue(left, left->getType());
        irBuilder.CreateStore(left, load->getPointerOperand());
        return left;        
    }
    case BinaryOp::mod_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateSRem(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_and_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateAnd(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_or_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateOr(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_xor_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateXor(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::left_shift_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateShl(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::right_shift_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        BinaryArithCastValue(left, right);
        llvm::Value *tmp = irBuilder.CreateAShr(left, right);
        AssignCastValue(tmp, left->getType());
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }                                    
    default:
        break;
    }
    return nullptr;
}

llvm::Value * CodeGen::VisitNumberExpr(NumberExpr *numberExpr) {
    if (numberExpr->ty->IsIntegerType()) {
        int bitCount = numberExpr->ty->GetSize() * 8;
        return irBuilder.getIntN(bitCount, numberExpr->value.v);
    }else {
        if (numberExpr->ty->GetKind() == CType::TY_Float) {
            return llvm::ConstantFP::get(irBuilder.getFloatTy(), numberExpr->value.d);
        }else {
            return llvm::ConstantFP::get(irBuilder.getDoubleTy(), numberExpr->value.d);
        }
    }
}

llvm::Value * CodeGen::VisitStringExpr(StringExpr *expr) {
    llvm::Constant *constant = llvm::ConstantDataArray::getString(context, expr->value);
    llvm::GlobalValue *g = new llvm::GlobalVariable(*module, constant->getType(), true, llvm::GlobalValue::PrivateLinkage, constant);
    return g;
}

llvm::Value * CodeGen::VisitBlockStmt(BlockStmt *p) {
    PushScope();
    for (const auto &stmt : p->nodeVec) {
        stmt->Accept(this);
        if (llvm::dyn_cast<ReturnStmt>(stmt.get()) ||
            llvm::dyn_cast<BreakStmt>(stmt.get()) ||
            llvm::dyn_cast<ContinueStmt>(stmt.get())) {
                break;
        }
    }
    PopScope();
    return nullptr;
}

llvm::Value * CodeGen::VisitDeclStmt(DeclStmt *p) {
    llvm::Value *lastVal = nullptr;
    for (const auto &node : p->nodeVec) {
        lastVal = node->Accept(this);
    }
    return lastVal;
}

/// 划分基本块 
llvm::Value * CodeGen::VisitIfStmt(IfStmt *p) {
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then");
    llvm::BasicBlock *elseBB = nullptr;
    if (p->elseNode)
        elseBB = llvm::BasicBlock::Create(context, "else");
    llvm::BasicBlock *lastBB = llvm::BasicBlock::Create(context, "last");

    llvm::Value *val = p->condNode->Accept(this);
    val = ConvertToBoolVal(val);
    irBuilder.CreateCondBr(val, thenBB, p->elseNode ? elseBB : lastBB);

    /// handle then bb
    thenBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(thenBB);
    p->thenNode->Accept(this);

    auto *tmpBB = irBuilder.GetInsertBlock();
    if (tmpBB->empty() || !tmpBB->back().isTerminator()) {
        irBuilder.CreateBr(lastBB);
    }

    if (elseBB) {
        elseBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(elseBB);
        p->elseNode->Accept(this);
        auto *tmpBB = irBuilder.GetInsertBlock();
        if (tmpBB->empty() || !tmpBB->back().isTerminator()) {
            irBuilder.CreateBr(lastBB);
        }
    }

    lastBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(lastBB);

    return nullptr;
}

llvm::Value * CodeGen::VisitForStmt(ForStmt *p) {
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for.cond");
    llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "for.inc");
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for.body");
    llvm::BasicBlock *lastBB = llvm::BasicBlock::Create(context, "for.last");

    breakBBs.insert({p, lastBB});
    continueBBs.insert({p, incBB});

    if (p->initNode) {
        p->initNode->Accept(this);
    }
    irBuilder.CreateBr(condBB);

    condBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(condBB);
    if (p->condNode) {
        llvm::Value *val = p->condNode->Accept(this);
        val = ConvertToBoolVal(val);
        irBuilder.CreateCondBr(val, bodyBB, lastBB);
    }else {
        irBuilder.CreateBr(bodyBB);
    }

    bodyBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(bodyBB);
    if (p->bodyNode) {
        p->bodyNode->Accept(this);
    }
    /// 更新body
    auto *tmpBB = irBuilder.GetInsertBlock();
    /// fix, for body 内部可能已经包含了终结指令
    if (tmpBB->empty() || !tmpBB->back().isTerminator()) {
        irBuilder.CreateBr(incBB);
    }

    incBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(incBB);
    if (p->incNode) {
        p->incNode->Accept(this);
    }
    irBuilder.CreateBr(condBB);

    lastBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(lastBB);

    breakBBs.erase(p);
    continueBBs.erase(p);

    return nullptr;
}

llvm::Value * CodeGen::VisitContinueStmt(ContinueStmt *p) {
    /// jump incBB
    llvm::BasicBlock *bb = continueBBs[p->target.get()];
    irBuilder.CreateBr(bb);
    return nullptr;
}

llvm::Value * CodeGen::VisitReturnStmt(ReturnStmt *p) {
    if (p->expr) {
        llvm::Value *val = p->expr->Accept(this);
        AssignCastValue(val, curFunc->getReturnType());
        return irBuilder.CreateRet(val);
    }else {
        return irBuilder.CreateRetVoid();
    }
}

llvm::Value * CodeGen::VisitBreakStmt(BreakStmt *p) {
    /// jump lastBB
    llvm::BasicBlock *bb = breakBBs[p->target.get()];
    irBuilder.CreateBr(bb);
    return nullptr;
}

llvm::Value * CodeGen::VisitSwitchStmt(SwitchStmt *p) {
    llvm::Value *val = p->expr->Accept(this);
    auto *defaultBB = llvm::BasicBlock::Create(context, "default");
    auto *thenBB = llvm::BasicBlock::Create(context, "then");
    auto *switchInst = irBuilder.CreateSwitch(val, defaultBB);

    breakBBs.insert({p, thenBB});
    switchStack.push_back(switchInst);

    /// 这里会有0到多个case语句，0到1个default语句
    p->stmt->Accept(this);

    /// 考虑最后的default语句
    /// 1. 没有 default stmt，插入一个默认的default stmt
    if (!p->defaultStmt) {
        defaultBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(defaultBB);
        irBuilder.CreateBr(thenBB);
    }
    //  2. default 是否有 terminate 指令
    if (defaultBB->empty() || !defaultBB->back().isTerminator()) {
        irBuilder.SetInsertPoint(defaultBB);
        irBuilder.CreateBr(thenBB);
    }

    thenBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(thenBB);

    breakBBs.erase(p);
    switchStack.pop_back();

    return nullptr;
}

llvm::Value * CodeGen::VisitCaseStmt(CaseStmt *p) {
    auto *switchInst = switchStack.back();
    llvm::Value *val = p->expr->Accept(this);
    AssignCastValue(val, switchInst->getCondition()->getType());

    /// case 语句需要新建一个基本块
    auto *caseBB = llvm::BasicBlock::Create(context);
    auto *constant = llvm::dyn_cast<llvm::ConstantInt>(val);
    if (!constant) {
        assert(0 && "expected constant expression in case");
    }

    /// 考虑上一个case 语句
    if (switchInst->getNumCases() > 0) {
        const auto &lastCase = switchInst->case_begin() + (switchInst->getNumCases() - 1);
        /// 获取当前case的最后一个基本块
        const auto &lastCaseBB = lastCase->getCaseSuccessor();
        if (lastCaseBB->empty() || !lastCaseBB->back().isTerminator()) {
            irBuilder.CreateBr(caseBB);
        }
    }

    switchInst->addCase(constant, caseBB);
    caseBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(caseBB);
    if (p->stmt) {
        p->stmt->Accept(this);
    }
    return nullptr;
}

llvm::Value * CodeGen::VisitDefaultStmt(DefaultStmt *p) {
    auto *switchInst = switchStack.back();
    /// 从之前的switch指令里面，获取到default dest
    auto *defaultBB = switchInst->getDefaultDest();

    /// 考虑最后一个case 语句
    if (switchInst->getNumCases() > 0) {
        const auto &lastCase = switchInst->case_begin() + (switchInst->getNumCases() - 1);
        const auto &lastCaseBB = lastCase->getCaseSuccessor();
        if (lastCaseBB->empty() || !lastCaseBB->back().isTerminator()) {
            irBuilder.CreateBr(defaultBB);
        }
    }

    defaultBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(defaultBB);
    p->stmt->Accept(this);
    return nullptr;
}

llvm::Value * CodeGen::VisitDoWhileStmt(DoWhileStmt *p) {
    
    auto *body = llvm::BasicBlock::Create(context, "body");
    auto *cond = llvm::BasicBlock::Create(context, "cond");
    auto *then = llvm::BasicBlock::Create(context, "then");

    breakBBs.insert({p, then});
    continueBBs.insert({p, cond});

    /// 生成body inst
    irBuilder.CreateBr(body);
    body->insertInto(curFunc);
    irBuilder.SetInsertPoint(body);
    p->stmt->Accept(this);

    auto *tmpBB = irBuilder.GetInsertBlock();
    if (tmpBB->empty() || !tmpBB->back().isTerminator()) {
        irBuilder.CreateBr(cond);
    }

    /// 生成cond inst
    cond->insertInto(curFunc);
    irBuilder.SetInsertPoint(cond);
    llvm::Value *val = p->expr->Accept(this);
    val = ConvertToBoolVal(val);
    irBuilder.CreateCondBr(val, body, then);

    /// 生成 then inst
    then->insertInto(curFunc);
    irBuilder.SetInsertPoint(then);

    breakBBs.erase(p);
    continueBBs.erase(p);

    return nullptr;
}

llvm::Value * CodeGen::VisitVariableDecl(VariableDecl *decl) {
    llvm::Type *ty = decl->ty->Accept(this);
    llvm::StringRef text(decl->tok.ptr, decl->tok.len);

    if (decl->isGlobal) {
        auto GetInitValueByOffset = [&](const std::vector<int> &offset) -> std::shared_ptr<VariableDecl::InitValue> {
            const auto &initVals = decl->initValues;
            for (const auto &n : initVals) {
                if (n->offsetList.size() != offset.size()) {
                    continue;
                }
                bool find = true;    
                for (int i = 0; i < offset.size(); ++i) {
                    if (n->offsetList[i] != offset[i]) {
                        find = false;
                        break;
                    }
                }
                if (find) {
                    return n;
                }
            }
            return nullptr;
        };

        auto GetInitialValue = [&](llvm::Type *ty, auto &&func, std::vector<int> offset)->llvm::Constant * {
            if (ty->isIntegerTy()) {
                std::shared_ptr<VariableDecl::InitValue> init = GetInitValueByOffset(offset);
                if (init) {
                    auto *c = init->value->Accept(this);
                    AssignCastValue(c, ty);
                    return llvm::dyn_cast<llvm::Constant>(c);
                }
                return irBuilder.getInt32(0);
            }else if (ty->isPointerTy()) {
                std::shared_ptr<VariableDecl::InitValue> init = GetInitValueByOffset(offset);
                if (init) {
                    auto *c = init->value->Accept(this);
                    AssignCastValue(c, ty);
                    return llvm::dyn_cast<llvm::Constant>(c);
                }
                return llvm::ConstantPointerNull::get(llvm::dyn_cast<llvm::PointerType>(ty));
            }else if (ty->isStructTy()) {
                llvm::StructType *structTy = llvm::dyn_cast<llvm::StructType>(ty);
                int size = structTy->getStructNumElements();
                llvm::SmallVector<llvm::Constant *> elemConstantVal;
                for (int i = 0; i < size; ++i) {
                    offset.push_back(i);
                    elemConstantVal.push_back(func(structTy->getStructElementType(i), func, offset));
                    offset.pop_back();
                }
                return llvm::ConstantStruct::get(structTy, elemConstantVal);
            }else if (ty->isArrayTy()) {
                llvm::ArrayType *arrTy = llvm::dyn_cast<llvm::ArrayType>(ty);
                int size = arrTy->getArrayNumElements();
                llvm::SmallVector<llvm::Constant *> elemConstantVal;
                for (int i = 0; i < size; ++i) {
                    offset.push_back(i);
                    elemConstantVal.push_back(func(arrTy->getArrayElementType(), func, offset));
                    offset.pop_back();
                }
                return llvm::ConstantArray::get(arrTy, elemConstantVal); 
            }else {
                return nullptr;
            }
        };

        llvm::GlobalVariable *globalVar = new llvm::GlobalVariable(*module, ty, false, llvm::GlobalValue::ExternalLinkage, nullptr, text);
        globalVar->setAlignment(llvm::Align(decl->ty->GetAlign()));
        globalVar->setInitializer(GetInitialValue(ty, GetInitialValue, {0}));
        AddGlobalVarToMap(globalVar, ty, text);
        return globalVar;
    }else {
        /// 要放入到entry bb里面
        llvm::IRBuilder<> tmp(&curFunc->getEntryBlock(), curFunc->getEntryBlock().begin());
        auto *alloc = tmp.CreateAlloca(ty, nullptr, text);
        alloc->setAlignment(llvm::Align(decl->ty->GetAlign()));
        AddLocalVarToMap(alloc, ty, text);

        if (decl->initValues.size() > 0) {
            if (decl->initValues.size() == 1) {
                llvm::Value *initValue = decl->initValues[0]->value->Accept(this);
                AssignCastValue(initValue, decl->initValues[0]->declType->Accept(this));
                irBuilder.CreateStore(initValue, alloc);
            }else {
                if (llvm::ArrayType *arrType = llvm::dyn_cast<llvm::ArrayType>(ty)) {
                    for (const auto &initValue : decl->initValues) {
                        llvm::SmallVector<llvm::Value *> vec;
                        for (auto &offset : initValue->offsetList) {
                            vec.push_back(irBuilder.getInt32(offset));
                        }
                        llvm::Value *addr = irBuilder.CreateInBoundsGEP(ty, alloc, vec);
                        llvm::Value *v = initValue->value->Accept(this);
                        AssignCastValue(v, initValue->declType->Accept(this));
                        irBuilder.CreateStore(v, addr);
                    }
                }else if (llvm::StructType *structType = llvm::dyn_cast<llvm::StructType>(ty)) {
                    CRecordType *cStructType = llvm::dyn_cast<CRecordType>(decl->ty.get());
                    TagKind tagKind = cStructType->GetTagKind();
                    if (tagKind == TagKind::kStruct) {
                        for (const auto &initValue : decl->initValues) {
                            llvm::SmallVector<llvm::Value *> vec;
                            for (auto &offset : initValue->offsetList) {
                                vec.push_back(irBuilder.getInt32(offset));
                            }
                            llvm::Value *addr = irBuilder.CreateInBoundsGEP(ty, alloc, vec);
                            llvm::Value *v = initValue->value->Accept(this);
                            AssignCastValue(v, initValue->declType->Accept(this));
                            irBuilder.CreateStore(v, addr);
                        }
                    }else {
                        assert(decl->initValues.size() == 1);
                        llvm::SmallVector<llvm::Value *> vec;
                        const auto &initValue = decl->initValues[0];
                        for (auto &offset : initValue->offsetList) {
                            vec.push_back(irBuilder.getInt32(offset));
                        }
                        llvm::Value *addr = irBuilder.CreateInBoundsGEP(ty, alloc, vec);
                        llvm::Value *v = initValue->value->Accept(this);
                        AssignCastValue(v, initValue->declType->Accept(this));
                        irBuilder.CreateStore(v, addr);
                    }
                }
                else {
                    assert(0);
                }
            }
        }
        return alloc;
    }
}

llvm::Value * CodeGen::VisitFuncDecl(FuncDecl *decl) {
    ClearVarScope();
    CFuncType *cFuncTy = llvm::dyn_cast<CFuncType>(decl->ty.get());
    const auto &params = cFuncTy->GetParams();
    llvm::Function *func = module->getFunction(cFuncTy->GetName());
    
    if (!func) {
        /// main 
        llvm::FunctionType * funcTy = llvm::dyn_cast<llvm::FunctionType>(decl->ty->Accept(this));
        func = Function::Create(funcTy, GlobalValue::ExternalLinkage, cFuncTy->GetName(), module.get());
        AddGlobalVarToMap(func, funcTy, cFuncTy->GetName());

        int i = 0;
        for (auto &arg : func->args()) {
            arg.setName(params[i++].name);
        }
    }
    if (!decl->blockStmt) {
        return nullptr;
    }

    BasicBlock *entryBB = BasicBlock::Create(context, "entry", func);
    irBuilder.SetInsertPoint(entryBB);
    /// 记录当前函数
    curFunc = func;

    PushScope();

    /// 存放变量的分配
    int i = 0;
    for (auto &arg : func->args()) {
        auto *alloc = irBuilder.CreateAlloca(arg.getType(), nullptr, params[i].name);
        alloc->setAlignment(llvm::Align(params[i].type->GetAlign()));
        irBuilder.CreateStore(&arg, alloc);

        AddLocalVarToMap(alloc, arg.getType(), params[i].name);

        i++;
    }


    decl->blockStmt->Accept(this);

    auto &block = curFunc->back();
    if (block.empty() || !block.back().isTerminator()) {
        if (cFuncTy->GetRetType()->GetKind() == CType::TY_Void) {
            irBuilder.CreateRetVoid();
        }else if (curFunc->getReturnType()->isIntegerTy()){
            irBuilder.CreateRet(irBuilder.getIntN(curFunc->getReturnType()->getIntegerBitWidth(), 0));
        }else if (curFunc->getReturnType()->isFloatTy()) {
            irBuilder.CreateRet(llvm::ConstantFP::get(irBuilder.getFloatTy(),1.0f));
        }else if (curFunc->getReturnType()->isDoubleTy()) {
            irBuilder.CreateRet(llvm::ConstantFP::get(irBuilder.getDoubleTy(),1.0));
        }else if (curFunc->getReturnType()->isPointerTy()) {
            irBuilder.CreateRet(llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(curFunc->getReturnType())));
        }else {
            assert(0 && "not return value");
        }
    }

    PopScope();

    // verifyFunction(*mFunc);

    if (verifyModule(*module, &llvm::outs())) {
        module->print(llvm::outs(), nullptr);
    }

    return nullptr;
}

llvm::Value * CodeGen::VisitUnaryExpr(UnaryExpr *expr) {
    llvm::Value *val = expr->node->Accept(this);
    llvm::Type *ty = expr->node->ty->Accept(this);

    switch (expr->op)
    {
    case UnaryOp::positive:
        return val;
    case UnaryOp::negative:{
        return irBuilder.CreateNeg(val);
    }
    case UnaryOp::logical_not: {
        llvm::Value *tmp = irBuilder.CreateICmpNE(val, irBuilder.getInt32(0));
        return irBuilder.CreateZExt(irBuilder.CreateNot(tmp), irBuilder.getInt32Ty());
    }
    case UnaryOp::bitwise_not:
        return irBuilder.CreateNot(val);
    case UnaryOp::addr:
        return llvm::dyn_cast<LoadInst>(val)->getPointerOperand();
    case UnaryOp::deref: {
        llvm::Type *nodeTy = expr->ty->Accept(this);
        return irBuilder.CreateLoad(nodeTy, val);
    }
    case UnaryOp::inc: {
        /// ++a => a+1 -> a;
        if (ty->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, val, {irBuilder.getInt32(1)});
            irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
            return newVal;
        }else if (ty->isIntegerTy()) {
            llvm::Value *newVal = irBuilder.CreateAdd(val, irBuilder.getInt32(1));
            irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
            return newVal;
        }else {
            assert(0);
            return nullptr;
        }
    }
    case UnaryOp::dec:{
        /// --a => a-1 -> a;
        if (ty->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, val, {irBuilder.getInt32(-1)});
            irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
            return newVal;
        }else if (ty->isIntegerTy()) {
            llvm::Value *newVal = irBuilder.CreateSub(val, irBuilder.getInt32(1));
            irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
            return newVal;
        }else {
            assert(0);
            return nullptr;
        }
    }                                           
    default:
        break;
    }
    return nullptr;
}

llvm::Value * CodeGen::VisitCastExpr(CastExpr *expr) {
    llvm::Type *ty = expr->targetType->Accept(this);
    llvm::Value *val = expr->node->Accept(this);
    AssignCastValue(val, ty);
    return val;
}

llvm::Value * CodeGen::VisitSizeOfExpr(SizeOfExpr *expr) {

    std::shared_ptr<CType> ty = nullptr;
    if (expr->type) {
        ty = expr->type;
    }else {
        ty = expr->node->ty;
    }
    return irBuilder.getInt32(ty->GetSize());
}

llvm::Value * CodeGen::VisitPostIncExpr(PostIncExpr *expr) {
    /// p++;
    /// p = p+1;
    llvm::Value *val = expr->left->Accept(this);
    llvm::Type *ty = expr->left->ty->Accept(this);

    if (ty->isPointerTy()) {
        /// p = p + 1
        llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, val, {irBuilder.getInt32(1)});
        irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
        return val;
    }else if (ty->isIntegerTy()) {
        llvm::Value *newVal = irBuilder.CreateAdd(val, irBuilder.getInt32(1));
        irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
        return val;
    }else {
        assert(0);
        return nullptr;
    }
}

llvm::Value * CodeGen::VisitPostDecExpr(PostDecExpr *expr) {
    llvm::Value *val = expr->left->Accept(this);
    llvm::Type *ty = expr->left->ty->Accept(this);

    if (ty->isPointerTy()) {
        /// p = p - 1
        llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, val, {irBuilder.getInt32(-1)});
        irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
        return val;
    }else if (ty->isIntegerTy()) {
        llvm::Value *newVal = irBuilder.CreateSub(val, irBuilder.getInt32(1));
        irBuilder.CreateStore(newVal, llvm::dyn_cast<LoadInst>(val)->getPointerOperand());
        return val;
    }else {
        assert(0);
        return nullptr;
    }
}

llvm::Value * CodeGen::VisitPostSubscript(PostSubscript *expr) {
    llvm::Type *ty = expr->ty->Accept(this);
    llvm::Value *left = expr->left->Accept(this);
    llvm::Value *offset = expr->node->Accept(this);
    
    llvm::Value *addr;
    if (left->getType()->isPointerTy()) {
        addr = irBuilder.CreateInBoundsGEP(ty, left, {offset});
    }else if (left->getType()->isArrayTy()){
        addr = irBuilder.CreateInBoundsGEP(ty, llvm::dyn_cast<LoadInst>(left)->getPointerOperand(), {offset});
    }else {
        assert(0);
    }
    return irBuilder.CreateLoad(ty, addr);
}

/// a.b 
/// a -> T
llvm::Value * CodeGen::VisitPostMemberDotExpr(PostMemberDotExpr *expr) {
    llvm::Value *leftValue = expr->left->Accept(this);
    llvm::Type *leftType = expr->left->ty->Accept(this);

    CRecordType *cLeftType = llvm::dyn_cast<CRecordType>(expr->left->ty.get());
    TagKind kind = cLeftType->GetTagKind();

    llvm::Type *memType = expr->member.ty->Accept(this);

    llvm::Value *zero = irBuilder.getInt32(0);
    if (kind == TagKind::kStruct) {
        llvm::Value *next = irBuilder.getInt32(expr->member.elemIdx);
        llvm::Value *memAddr = irBuilder.CreateInBoundsGEP(leftType, llvm::dyn_cast<LoadInst>(leftValue)->getPointerOperand(), {zero, next});
        return irBuilder.CreateLoad(memType, memAddr);
    }else {
        llvm::Value *memAddr = irBuilder.CreateInBoundsGEP(leftType, llvm::dyn_cast<LoadInst>(leftValue)->getPointerOperand(), {zero, zero});
        llvm::Value *cast = irBuilder.CreateBitCast(memAddr, llvm::PointerType::getUnqual(memType));
        return irBuilder.CreateLoad(memType, cast);
    }
}

/// a->b
/// ptr* -> ptr
llvm::Value * CodeGen::VisitPostMemberArrowExpr(PostMemberArrowExpr *expr) {
    llvm::Value *leftValue = expr->left->Accept(this);
    llvm::Type *leftType = expr->left->ty->Accept(this);
    CPointType *cLeftPointerType = llvm::dyn_cast<CPointType>(expr->left->ty.get());
    
    CRecordType *cLeftType = llvm::dyn_cast<CRecordType>(cLeftPointerType->GetBaseType().get());
    TagKind kind = cLeftType->GetTagKind();

    llvm::Type *memType = expr->member.ty->Accept(this);

    llvm::Value *zero = irBuilder.getInt32(0);
    if (kind == TagKind::kStruct) {
        llvm::Value *next = irBuilder.getInt32(expr->member.elemIdx);
        llvm::Value *memAddr = irBuilder.CreateInBoundsGEP(cLeftPointerType->GetBaseType()->Accept(this), leftValue, {zero, next});
        return irBuilder.CreateLoad(memType, memAddr);
    }else {
        llvm::Value *memAddr = irBuilder.CreateInBoundsGEP(cLeftPointerType->GetBaseType()->Accept(this), leftValue, {zero, zero});
        llvm::Value *cast = irBuilder.CreateBitCast(memAddr, llvm::PointerType::getUnqual(memType));
        return irBuilder.CreateLoad(memType, cast);
    }
}

llvm::Value * CodeGen::VisitPostFuncCall(PostFuncCall *expr) {
    llvm::Value *funcArr = expr->left->Accept(this);
    llvm::Type *ty = expr->left->ty->Accept(this);
    llvm::FunctionType *funcTy = llvm::dyn_cast<llvm::FunctionType>(expr->left->ty->Accept(this));
    CFuncType *cFuncTy = llvm::dyn_cast<CFuncType>(expr->left->ty.get());
    
    const auto &param = cFuncTy->GetParams();

    int i = 0;
    llvm::SmallVector<llvm::Value *> args;
    for (const auto &arg : expr->args) {
        llvm::Value *val = arg->Accept(this);
        if (i < param.size()) {
            AssignCastValue(val, param[i].type->Accept(this));
        }
        /// fix var param
        if (val->getType()->isArrayTy()) {
            auto *load = llvm::dyn_cast<llvm::LoadInst>(val);
            if (load) {
                val = load->getPointerOperand();
            }else {
                llvm::ArrayType *pty = llvm::dyn_cast<llvm::ArrayType>(val->getType());
                llvm::Value *zero = irBuilder.getInt32(0);
                val = irBuilder.CreateInBoundsGEP(pty->getArrayElementType(), val, {zero});
            }
        }
        args.push_back(val);
        ++i;
    }
    return irBuilder.CreateCall(funcTy, funcArr, args);
}

llvm::Value * CodeGen::VisitThreeExpr(ThreeExpr *expr) {
    llvm::Value *val = expr->cond->Accept(this);
    val = ConvertToBoolVal(val);

    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then");
    llvm::BasicBlock *elsBB = llvm::BasicBlock::Create(context, "els");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "merge");
    irBuilder.CreateCondBr(val, thenBB, elsBB);

    thenBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(thenBB);
    llvm::Value *thenVal = expr->then->Accept(this);
    auto *thenLastBB = irBuilder.GetInsertBlock();
    irBuilder.CreateBr(mergeBB);

    elsBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(elsBB);
    llvm::Value *elsVal = expr->els->Accept(this);
    auto *elsLastBB = irBuilder.GetInsertBlock();
    irBuilder.CreateBr(mergeBB);

    mergeBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(mergeBB);
    BinaryArithCastValue(thenVal, elsVal);
    llvm::PHINode *phi = irBuilder.CreatePHI(expr->then->ty->Accept(this), 2);
    phi->addIncoming(thenVal, thenLastBB);
    phi->addIncoming(elsVal, elsLastBB);
    return phi;
}

/// alloc T -> T *
/// load T* -> T
llvm::Value * CodeGen::VisitVariableAccessExpr(VariableAccessExpr *expr) {
    llvm::StringRef text(expr->tok.ptr, expr->tok.len);
    const auto &[addr, ty] = GetVarByName(text);
    
    if (ty->isFunctionTy()) {
        return addr;
    }

    return irBuilder.CreateLoad(ty, addr, text);
}

llvm::Type * CodeGen::VisitPrimaryType(CPrimaryType *ty) {
    if (ty->GetKind() == CType::TY_Void) {
        return irBuilder.getVoidTy();
    }
    if (ty->IsIntegerType()) {
        int bitCount = ty->GetSize() * 8;
        return irBuilder.getIntNTy(bitCount);
    }
    if (ty->GetKind() == CType::TY_Float) {
        return irBuilder.getFloatTy();
    } else if (ty->GetKind() == CType::TY_Double || ty->GetKind() == CType::TY_LDouble) {
        return irBuilder.getDoubleTy();
    }
    assert(0);
    return nullptr;
}

llvm::Type * CodeGen::VisitPointType(CPointType *ty) {
    llvm::Type *baseType = ty->GetBaseType()->Accept(this);
    return llvm::PointerType::getUnqual(baseType);
}

llvm::Type * CodeGen::VisitArrayType(CArrayType *ty) {
    llvm::Type *elementType = ty->GetElementType()->Accept(this);
    return llvm::ArrayType::get(elementType, ty->GetElementCount());
}

llvm::Type * CodeGen::VisitRecordType(CRecordType *ty) {
    llvm::StructType *structType = nullptr;
    structType = llvm::StructType::getTypeByName(context, ty->GetName());
    if (structType) {
        return structType;
    }
    structType = llvm::StructType::create(context, ty->GetName());
    // structType->setName(ty->GetName());

    TagKind tagKind = ty->GetTagKind();

    if (tagKind == TagKind::kStruct) {
        llvm::SmallVector<llvm::Type *> vec;
        for (const auto &m : ty->GetMembers()) {
            vec.push_back(m.ty->Accept(this));
        }
        structType->setBody(vec);
    }else {
        llvm::SmallVector<llvm::Type *> vec;
        const auto &members = ty->GetMembers();
        int idx = ty->GetMaxElementIdx();
        structType->setBody(members[idx].ty->Accept(this));
    }
    // structType->print(llvm::outs());
    return structType;
}

llvm::Type * CodeGen::VisitFuncType(CFuncType *ty) {
    llvm::Type *retTy = ty->GetRetType()->Accept(this);
    llvm::SmallVector<llvm::Type *> argsType;
    for (const auto &arg : ty->GetParams()) {
        argsType.push_back(arg.type->Accept(this));
    }
    return llvm::FunctionType::get(retTy, argsType, ty->IsVarArg());
}


void CodeGen::AddLocalVarToMap(llvm::Value *addr, llvm::Type *ty, llvm::StringRef name) {
    localVarMap.back().insert({name, {addr, ty}});
}

void CodeGen::AddGlobalVarToMap(llvm::Value *addr, llvm::Type *ty, llvm::StringRef name) {
    globalVarMap.insert({name, {addr, ty}});
}

std::pair<llvm::Value *, llvm::Type *> CodeGen::GetVarByName(llvm::StringRef name) {
    for (auto it = localVarMap.rbegin(); it != localVarMap.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return (*it)[name];
        }
    }
    assert(globalVarMap.find(name) != globalVarMap.end());
    return globalVarMap[name];
}

void CodeGen::PushScope() {
    localVarMap.emplace_back();
}

void CodeGen::PopScope() {
    localVarMap.pop_back();
}

void CodeGen::ClearVarScope() {
    localVarMap.clear();
}

void CodeGen::AssignCastValue(llvm::Value *&val, llvm::Type *destTy) {
    if (val->getType() != destTy) {
        if (val->getType()->isIntegerTy()) {
            if (destTy->isIntegerTy()) {
                val = irBuilder.CreateIntCast(val, destTy, true);
            }
            else if (destTy->isFloatingPointTy()) {
                val = irBuilder.CreateSIToFP(val, destTy);
            }
            else if (destTy->isPointerTy()) {
                if (val->getType()->getIntegerBitWidth() != 64) {
                    val = irBuilder.CreateIntCast(val, irBuilder.getInt64Ty(), true);
                }
                val = irBuilder.CreateIntToPtr(val, destTy);
            }else {
                assert(0 && "connot convert type");
            }
        }else if (val->getType()->isFloatingPointTy()) {
            if (destTy->isFloatingPointTy()) {
                val = irBuilder.CreateFPCast(val, destTy);
            }else if (destTy->isIntegerTy()) {
                val = irBuilder.CreateFPToSI(val, destTy);
            }else {
                assert(0 && "connot convert type");
            }
        }
        else if (val->getType()->isPointerTy()) {
            if (destTy->isIntegerTy()) {
                val = irBuilder.CreatePtrToInt(val, destTy);
            }else {
                assert(0 && "connot convert type");
            }
        }else if (val->getType()->isArrayTy()) {
            if (destTy->isIntOrPtrTy()) {
                auto *load = llvm::dyn_cast<llvm::LoadInst>(val);
                if (load) {
                    val = load->getPointerOperand();
                }else {
                    llvm::ArrayType *pty = llvm::dyn_cast<llvm::ArrayType>(val->getType());
                    llvm::Value *zero = irBuilder.getInt32(0);
                    val = irBuilder.CreateInBoundsGEP(pty->getArrayElementType(), val, {zero});
                }
                if (destTy->isIntegerTy()) {
                    val = irBuilder.CreatePtrToInt(val, destTy);
                }
            }else {
                assert(0 && "connot convert type");
            }
        }
    }
}

void CodeGen::BinaryArithCastValue(llvm::Value *&left, llvm::Value *&right) {
    auto CastToDouble = [&](llvm::Value *&value) {
        if (!value->getType()->isDoubleTy()) {
            if (value->getType()->isIntegerTy()) {
                value = irBuilder.CreateSIToFP(value, irBuilder.getDoubleTy());
            }else if (value->getType()->isFloatingPointTy()) {
                value = irBuilder.CreateFPCast(value, irBuilder.getDoubleTy());
            }else {
                assert(0 && "connot convert type");
            }
        }
    };
    auto CastToFloat = [&](llvm::Value *&value) {
        if (!value->getType()->isFloatTy()) {
            if (value->getType()->isIntegerTy()) {
                value = irBuilder.CreateSIToFP(value, irBuilder.getFloatTy());
            }else if (value->getType()->isFloatingPointTy()) {
                value = irBuilder.CreateFPCast(value, irBuilder.getFloatTy());
            }else {
                assert(0 && "connot convert type");
            }
        }
    };

    if (left->getType()->isDoubleTy() || right->getType()->isDoubleTy()) {
        CastToDouble(left);
        CastToDouble(right);
    }else if (left->getType()->isFloatTy() || right->getType()->isFloatTy()) {
        CastToFloat(left);
        CastToFloat(right);
    }else if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
        if (left->getType()->getIntegerBitWidth() < 32u || left->getType()->getIntegerBitWidth() < right->getType()->getIntegerBitWidth()) {
            left = irBuilder.CreateIntCast(left, irBuilder.getIntNTy(std::max(32u, right->getType()->getIntegerBitWidth())), true);
        }

        if (right->getType()->getIntegerBitWidth() < 32u || right->getType()->getIntegerBitWidth() < left->getType()->getIntegerBitWidth()) {
            right = irBuilder.CreateIntCast(right, irBuilder.getIntNTy(std::max(32u, left->getType()->getIntegerBitWidth())), true);
        }
    }else if (!left->getType()->isPointerTy() || !right->getType()->isPointerTy()) {
       assert(0 && "connot convert type");
    }
}

llvm::Value *CodeGen::ConvertToBoolVal(llvm::Value *val) {
    if (val->getType()->isIntegerTy()) {
        if (val->getType()->getIntegerBitWidth() > 1) {
            return irBuilder.CreateICmpNE(val, irBuilder.getIntN(val->getType()->getIntegerBitWidth(), 0));
        }
        return val;
    }else if (val->getType()->isFloatingPointTy()) {
        return irBuilder.CreateFCmpUNE(val, llvm::ConstantFP::get(val->getType(), 0));
    }else if (val->getType()->isPointerTy()) {
        return irBuilder.CreateICmpNE(val, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(val->getType())));
    }else {
        return nullptr;
    }
}