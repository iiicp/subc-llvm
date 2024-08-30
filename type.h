#pragma once
#include <memory>
#include "llvm/IR/Type.h"

class CPrimaryType;
class CPointType;
class CArrayType;
class CRecordType;
class CFuncType;

class TypeVisitor {
public:
    virtual ~TypeVisitor() {}
    virtual llvm::Type * VisitPrimaryType(CPrimaryType *ty) = 0;
    virtual llvm::Type * VisitPointType(CPointType *ty) = 0;
    virtual llvm::Type * VisitArrayType(CArrayType *ty) = 0;
    virtual llvm::Type * VisitRecordType(CRecordType *ty) = 0;
    virtual llvm::Type * VisitFuncType(CFuncType *ty) = 0;
};

enum class TagKind {
    kStruct,
    kUnion
};

class CType {
public:
    enum Kind {
        TY_Void,
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
        TY_LDouble,
        TY_Point,
        TY_Array,
        TY_Record,
        TY_Func,
    };
protected:
    Kind kind;
    int size;       /// 字节数
    int align;      /// 对齐数
    bool sign{true}; ///默认是有符号
public:
    CType(Kind kind, int size, int align, bool sign = true):kind(kind), size(size), align(align), sign(sign) {}
    virtual ~CType() {}
    const Kind GetKind() const {return kind;}
    const int GetSize() const {
        return size;
    }
    const int GetAlign() const {
        return align;
    }

    bool IsIntegerType();
    bool IsFloatType();
    bool IsArithType();

    virtual llvm::Type * Accept(TypeVisitor *v) {return nullptr;}

    static std::shared_ptr<CType> VoidType;
    static std::shared_ptr<CType> CharType;
    static std::shared_ptr<CType> UCharType;
    static std::shared_ptr<CType> ShortType;
    static std::shared_ptr<CType> UShortType;
    static std::shared_ptr<CType> IntType;
    static std::shared_ptr<CType> UIntType;
    static std::shared_ptr<CType> LongType;
    static std::shared_ptr<CType> ULongType;
    static std::shared_ptr<CType> LongLongType;
    static std::shared_ptr<CType> ULongLongType;
    static std::shared_ptr<CType> FloatType;
    static std::shared_ptr<CType> DoubleType;
    static std::shared_ptr<CType> LDoubleType;

    static llvm::StringRef GenAnonyRecordName(TagKind tagKind);
};

class CPrimaryType : public CType {
public:
    CPrimaryType(Kind kind, int size, int align, bool sign):CType(kind, size, align, sign) {}
    
    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitPrimaryType(this);
    }
    
    static bool classof(const CType *ty) {
        return (ty->GetKind() == TY_Char || ty->GetKind() == TY_UChar || 
        ty->GetKind() == TY_Short || ty->GetKind() == TY_UShort ||
        ty->GetKind() == TY_Int || ty->GetKind() == TY_UInt || 
        ty->GetKind() == TY_Long || ty->GetKind() == TY_ULong || 
        ty->GetKind() == TY_LLong || ty->GetKind() == TY_ULLong || 
        ty->GetKind() == TY_Float || ty->GetKind() == TY_Double);
    }
};

class CPointType : public CType{
private:
    std::shared_ptr<CType> baseType;
public:
    CPointType(std::shared_ptr<CType> baseType):CType(Kind::TY_Point, 8, 8), baseType(baseType) {}
    
    std::shared_ptr<CType> GetBaseType() {
        return baseType;
    }

    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitPointType(this);
    }

    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Point;
    }
};

class CArrayType : public CType {
private:
    std::shared_ptr<CType> elementType;
    int elementCount{-1};
public:
    CArrayType(std::shared_ptr<CType> elementType, int elementCount)
    :CType(Kind::TY_Array, elementCount * elementType->GetSize(), elementType->GetAlign()), elementType(elementType), elementCount(elementCount) {}
    
    std::shared_ptr<CType> GetElementType() {
        return elementType;
    }

    const int GetElementCount() {
        return elementCount;
    }

    void SetElementCount(int count) {
        elementCount = count;
        this->size = elementCount * elementType->GetSize();
    }

    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitArrayType(this);
    }

    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Array;
    }
};

struct Member {
    std::shared_ptr<CType> ty;
    llvm::StringRef name;
    int offset;
    int elemIdx;
};


class CRecordType : public CType {
private:
    llvm::StringRef name;
    std::vector<Member> members;
    TagKind tagKind;
    int maxElementIdx;
public:
    CRecordType(llvm::StringRef name, const std::vector<Member> &members, TagKind tagKind);

    const llvm::StringRef GetName() {
        return name;
    }

    const std::vector<Member> &GetMembers() {
        return members;
    }

    void SetMembers(const std::vector<Member>& members);

    const TagKind GetTagKind() {
        return tagKind;
    }

    int GetMaxElementIdx() {
        return maxElementIdx;
    }

    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitRecordType(this);
    }

    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Record;
    }
private:
    void UpdateStructOffset();
    void UpdateUnionOffset();
};

struct Param {
    std::shared_ptr<CType> type;
    llvm::StringRef name;
};

class CFuncType : public CType {
private:
    std::shared_ptr<CType> retType;
    std::vector<Param> params;
    llvm::StringRef name;
    bool isVarArg{false};
public:
    bool hasBody{false};
    CFuncType(std::shared_ptr<CType> retType, const std::vector<Param>& params, llvm::StringRef name, bool isVarArg);
    
    const llvm::StringRef GetName() {
        return name;
    }

    const std::vector<Param> &GetParams() {
        return params;
    }

    std::shared_ptr<CType> GetRetType() {
        return retType;
    }

    bool IsVarArg() {
        return isVarArg;
    }

    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitFuncType(this);
    }

    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Func;
    }
};