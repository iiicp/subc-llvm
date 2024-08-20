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
        llvm::Type *ty = binaryExpr->left->ty->Accept(this);
        if (ty->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, left, {right});
            return newVal;
        }else {
            return irBuilder.CreateNSWAdd(left, right);
        }
    }
    case BinaryOp::sub:{
        llvm::Type *ty = binaryExpr->left->ty->Accept(this);
        if (ty->isPointerTy()) {
            llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, left, {irBuilder.CreateNeg(right)});
            return newVal;
        }else {
            return irBuilder.CreateNSWSub(left, right);
        }
    }
    case BinaryOp::mul:{
        return irBuilder.CreateNSWMul(left, right);
    }
    case BinaryOp::div:{
        return irBuilder.CreateSDiv(left, right);
    }
    case BinaryOp::mod: {
        return irBuilder.CreateSRem(left, right);
    }
    case BinaryOp::bitwise_and:{      
        return irBuilder.CreateAnd(left, right);
    }
    case BinaryOp::bitwise_or:{      
        return irBuilder.CreateOr(left, right);
    }
    case BinaryOp::bitwise_xor:{     
        return irBuilder.CreateXor(left, right);
    }   
    case BinaryOp::left_shift:{    
        return irBuilder.CreateShl(left, right);
    }
    case BinaryOp::right_shift:{  
        return irBuilder.CreateAShr(left, right);
    }        
    case BinaryOp::equal: {      
        /// getInt1Ty()
        llvm::Value *val = irBuilder.CreateICmpEQ(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::not_equal:{      
        llvm::Value *val = irBuilder.CreateICmpNE(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::less:{     
        llvm::Value *val = irBuilder.CreateICmpSLT(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::less_equal:{      
        llvm::Value *val = irBuilder.CreateICmpSLE(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }      
    case BinaryOp::greater:{      
        llvm::Value *val = irBuilder.CreateICmpSGT(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }
    case BinaryOp::greater_equal:{ 
        llvm::Value *val = irBuilder.CreateICmpSGE(left, right);
        return irBuilder.CreateIntCast(val, irBuilder.getInt32Ty(), true);
    }  
    case BinaryOp::logical_and:{
        /// A && B

        llvm::BasicBlock *nextBB = llvm::BasicBlock::Create(context, "nextBB", curFunc);
        llvm::BasicBlock *falseBB = llvm::BasicBlock::Create(context, "falseBB");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "mergeBB");

        llvm::Value *left = binaryExpr->left->Accept(this);
        CastValue(left, irBuilder.getInt32Ty());
        llvm::Value *val = irBuilder.CreateICmpNE(left, irBuilder.getInt32(0));
        irBuilder.CreateCondBr(val, nextBB, falseBB);

        irBuilder.SetInsertPoint(nextBB);
        llvm::Value *right = binaryExpr->right->Accept(this);
        right = irBuilder.CreateICmpNE(right, irBuilder.getInt32(0));
        /// 32位 0 或着 1
        right = irBuilder.CreateZExt(right, irBuilder.getInt32Ty());
        irBuilder.CreateBr(mergeBB);

        /// right 这个值，所在的基本块，并不一定是 之前的nextBB了.
        /// 原因是：binaryExpr->right->Accept(this) 内部会生成新的基本块

        /// 拿到当前插入的block, 建立一个值和基本块的关系 {right, nextBB}
        nextBB = irBuilder.GetInsertBlock();
        
        falseBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(falseBB);
        irBuilder.CreateBr(mergeBB);

        mergeBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(mergeBB);
        llvm::PHINode *phi = irBuilder.CreatePHI(irBuilder.getInt32Ty(), 2);
        phi->addIncoming(right, nextBB);
        phi->addIncoming(irBuilder.getInt32(0), falseBB);

        return phi;
    }  
    case BinaryOp::logical_or: {
        /// A || B && C

        llvm::BasicBlock *nextBB = llvm::BasicBlock::Create(context, "nextBB", curFunc);
        llvm::BasicBlock *trueBB = llvm::BasicBlock::Create(context, "trueBB");
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "mergeBB");

        llvm::Value *left = binaryExpr->left->Accept(this);
        CastValue(left, irBuilder.getInt32Ty());
        llvm::Value *val = irBuilder.CreateICmpNE(left, irBuilder.getInt32(0));
        irBuilder.CreateCondBr(val, trueBB, nextBB);

        irBuilder.SetInsertPoint(nextBB);
        /// 右子树内部也生成了基本块
        llvm::Value *right = binaryExpr->right->Accept(this);
        right = irBuilder.CreateICmpNE(right, irBuilder.getInt32(0));
        /// 32位 0 或着 1
        right = irBuilder.CreateZExt(right, irBuilder.getInt32Ty());
        irBuilder.CreateBr(mergeBB);
        /// right 这个值，所在的基本块，并不一定是 之前的nextBB了.
        /// 原因是：binaryExpr->right->Accept(this) 内部会生成新的基本块

        /// 拿到当前插入的block, 建立一个值和基本块的关系 {right, nextBB}
        nextBB = irBuilder.GetInsertBlock();

        trueBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(trueBB);
        irBuilder.CreateBr(mergeBB);

        mergeBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(mergeBB);
        llvm::PHINode *phi = irBuilder.CreatePHI(irBuilder.getInt32Ty(), 2);
        phi->addIncoming(right, nextBB);
        phi->addIncoming(irBuilder.getInt32(1), trueBB);

        return phi;
    }
    case BinaryOp::assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        irBuilder.CreateStore(right, load->getPointerOperand());
        return right;
    }
    case BinaryOp::add_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Type *ty = binaryExpr->left->ty->Accept(this);
        if (ty->isPointerTy()) {
             llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, left, {right});
             irBuilder.CreateStore(newVal, load->getPointerOperand());
             return newVal;
        }else {
            /// a+=3; => a = a + 3;
            llvm::Value *tmp = irBuilder.CreateAdd(left, right);
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }
    }
    case BinaryOp::sub_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Type *ty = binaryExpr->left->ty->Accept(this);
        if (ty->isPointerTy()) {
             llvm::Value *newVal = irBuilder.CreateInBoundsGEP(ty, left, {irBuilder.CreateNeg(right)});
             irBuilder.CreateStore(newVal, load->getPointerOperand());
             return newVal;
        }else {
            llvm::Value *tmp = irBuilder.CreateSub(left, right);
            irBuilder.CreateStore(tmp, load->getPointerOperand());
            return tmp;
        }
    }
    case BinaryOp::mul_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateMul(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::div_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateSDiv(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;        
    }
    case BinaryOp::mod_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateSRem(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_and_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateAnd(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_or_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateOr(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::bitwise_xor_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateXor(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::left_shift_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateShl(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }
    case BinaryOp::right_shift_assign: {
        llvm::LoadInst *load = llvm::dyn_cast<llvm::LoadInst>(left);
        assert(load);
        llvm::Value *tmp = irBuilder.CreateAShr(left, right);
        irBuilder.CreateStore(tmp, load->getPointerOperand());
        return tmp;
    }                                    
    default:
        break;
    }
    return nullptr;
}

llvm::Value * CodeGen::VisitNumberExpr(NumberExpr *numberExpr) {
    return irBuilder.getInt32(numberExpr->tok.value);
}

llvm::Value * CodeGen::VisitBlockStmt(BlockStmt *p) {
    llvm::Value *lastVal = nullptr;
    for (const auto &stmt : p->nodeVec) {
        lastVal = stmt->Accept(this);
    }
    return lastVal;
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
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "cond", curFunc);
    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then");
    llvm::BasicBlock *elseBB = nullptr;
    if (p->elseNode)
        elseBB = llvm::BasicBlock::Create(context, "else");
    llvm::BasicBlock *lastBB = llvm::BasicBlock::Create(context, "last");

    irBuilder.CreateBr(condBB);
    irBuilder.SetInsertPoint(condBB);
    llvm::Value *val = p->condNode->Accept(this);
    CastValue(val, irBuilder.getInt32Ty());
    /// 整型比较指令
    llvm::Value *condVal = irBuilder.CreateICmpNE(val, irBuilder.getInt32(0));
    if (p->elseNode) {
        irBuilder.CreateCondBr(condVal, thenBB, elseBB);

        /// handle then bb
        thenBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(thenBB);
        p->thenNode->Accept(this);
        irBuilder.CreateBr(lastBB);

        elseBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(elseBB);
        p->elseNode->Accept(this);
        irBuilder.CreateBr(lastBB);
    }else {
        irBuilder.CreateCondBr(condVal, thenBB, lastBB);

        /// handle then bb
        thenBB->insertInto(curFunc);
        irBuilder.SetInsertPoint(thenBB);
        p->thenNode->Accept(this);
        irBuilder.CreateBr(lastBB);
    }

    lastBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(lastBB);

    return nullptr;
}

llvm::Value * CodeGen::VisitForStmt(ForStmt *p) {
    llvm::BasicBlock *initBB = llvm::BasicBlock::Create(context, "for.init", curFunc);
    llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "for.cond");
    llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "for.inc");
    llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "for.body");
    llvm::BasicBlock *lastBB = llvm::BasicBlock::Create(context, "for.last");

    breakBBs.insert({p, lastBB});
    continueBBs.insert({p, incBB});

    irBuilder.CreateBr(initBB);
    irBuilder.SetInsertPoint(initBB);
    if (p->initNode) {
        p->initNode->Accept(this);
    }
    irBuilder.CreateBr(condBB);

    condBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(condBB);
    if (p->condNode) {
        llvm::Value *val = p->condNode->Accept(this);
        CastValue(val, irBuilder.getInt32Ty());
        llvm::Value *condVal = irBuilder.CreateICmpNE(val, irBuilder.getInt32(0));
        irBuilder.CreateCondBr(condVal, bodyBB, lastBB);
    }else {
        irBuilder.CreateBr(bodyBB);
    }

    bodyBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(bodyBB);
    if (p->bodyNode) {
        p->bodyNode->Accept(this);
    }
    irBuilder.CreateBr(incBB);

    incBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(incBB);
    if (p->incNode) {
        p->incNode->Accept(this);
    }
    irBuilder.CreateBr(condBB);

    breakBBs.erase(p);
    continueBBs.erase(p);

    lastBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(lastBB);

    return nullptr;
}

llvm::Value * CodeGen::VisitContinueStmt(ContinueStmt *p) {
    /// jump incBB
    llvm::BasicBlock *bb = continueBBs[p->target.get()];
    irBuilder.CreateBr(bb);

    llvm::BasicBlock *out = llvm::BasicBlock::Create(context, "for.continue.death", curFunc);
    irBuilder.SetInsertPoint(out);
    return nullptr;
}

llvm::Value * CodeGen::VisitReturnStmt(ReturnStmt *p) {
    if (p->expr) {
        llvm::Value *val = p->expr->Accept(this);
        return irBuilder.CreateRet(val);
    }else {
        return irBuilder.CreateRetVoid();
    }
}

llvm::Value * CodeGen::VisitBreakStmt(BreakStmt *p) {
    /// jump lastBB
    llvm::BasicBlock *bb = breakBBs[p->target.get()];
    irBuilder.CreateBr(bb);

    llvm::BasicBlock *out = llvm::BasicBlock::Create(context, "for.break.death", curFunc);
    irBuilder.SetInsertPoint(out);
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
                    CastValue(c, ty);
                    return llvm::dyn_cast<llvm::Constant>(c);
                }
                return irBuilder.getInt32(0);
            }else if (ty->isPointerTy()) {
                std::shared_ptr<VariableDecl::InitValue> init = GetInitValueByOffset(offset);
                if (init) {
                    auto *c = init->value->Accept(this);
                    CastValue(c, ty);
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
                CastValue(initValue, decl->initValues[0]->declType->Accept(this));
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
                        CastValue(v, initValue->declType->Accept(this));
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
                            CastValue(v, initValue->declType->Accept(this));
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
                        CastValue(v, initValue->declType->Accept(this));
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
        }else {
            irBuilder.CreateRet(irBuilder.getInt32(0));
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
    llvm::FunctionType *funcTy = llvm::dyn_cast<llvm::FunctionType>(expr->left->ty->Accept(this));
    CFuncType *cFuncTy = llvm::dyn_cast<CFuncType>(expr->left->ty.get());
    
    const auto &param = cFuncTy->GetParams();

    int i = 0;
    llvm::SmallVector<llvm::Value *> args;
    for (const auto &arg : expr->args) {
        llvm::Value *val = arg->Accept(this);
        CastValue(val, param[i].type->Accept(this));
        args.push_back(val);
        ++i;
    }
    return irBuilder.CreateCall(funcTy, funcArr, args);
}

llvm::Value * CodeGen::VisitThreeExpr(ThreeExpr *expr) {
    llvm::Value *val = expr->cond->Accept(this);
    CastValue(val, irBuilder.getInt32Ty());
    llvm::Value *cond = irBuilder.CreateICmpNE(val, irBuilder.getInt32(0));

    llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", curFunc);
    llvm::BasicBlock *elsBB = llvm::BasicBlock::Create(context, "els");
    llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "merge");
    irBuilder.CreateCondBr(cond, thenBB, elsBB);

    irBuilder.SetInsertPoint(thenBB);
    llvm::Value *thenVal = expr->then->Accept(this);
    thenBB = irBuilder.GetInsertBlock();
    irBuilder.CreateBr(mergeBB);

    elsBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(elsBB);
    llvm::Value *elsVal = expr->els->Accept(this);
    elsBB = irBuilder.GetInsertBlock();
    irBuilder.CreateBr(mergeBB);

    mergeBB->insertInto(curFunc);
    irBuilder.SetInsertPoint(mergeBB);

    llvm::PHINode *phi = irBuilder.CreatePHI(expr->then->ty->Accept(this), 2);
    phi->addIncoming(thenVal, thenBB);
    phi->addIncoming(elsVal, elsBB);
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
    if (ty->GetKind() == CType::TY_Int) {
        return irBuilder.getInt32Ty();
    }else if (ty->GetKind() == CType::TY_Void) {
        return irBuilder.getVoidTy();
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
    return llvm::FunctionType::get(retTy, argsType, false);
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

void CodeGen::CastValue(llvm::Value *&val, llvm::Type *destTy) {
    if (val->getType() != destTy) {
        if (val->getType()->isIntegerTy()) {
            if (destTy->isPointerTy()) {
                val = irBuilder.CreateIntToPtr(val, destTy);
            }
        }else if (val->getType()->isPointerTy()) {
            if (destTy->isIntegerTy()) {
                val = irBuilder.CreatePtrToInt(val, destTy);
            }
        }else if (val->getType()->isArrayTy()) {
            if (destTy->isPointerTy()) {
                auto *load = llvm::dyn_cast<llvm::LoadInst>(val);
                val = load->getPointerOperand();
            }
        }
    }
}