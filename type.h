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
        TY_Int,
        TY_Point,
        TY_Array,
        TY_Record,
        TY_Func,
    };
protected:
    Kind kind;
    int size;       /// 字节数
    int align;      /// 对齐数
public:
    CType(Kind kind, int size, int align):kind(kind), size(size), align(align) {}
    virtual ~CType() {}
    const Kind GetKind() const {return kind;}
    const int GetSize() const {
        return size;
    }
    const int GetAlign() const {
        return align;
    }
    virtual llvm::Type * Accept(TypeVisitor *v) {return nullptr;}

    static std::shared_ptr<CType> IntType;

    static llvm::StringRef GenAnonyRecordName(TagKind tagKind);
};

class CPrimaryType : public CType {
public:
    CPrimaryType(Kind kind, int size, int align):CType(kind, size, align) {}
    
    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitPrimaryType(this);
    }
    
    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Int;
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
    int elementCount;
public:
    CArrayType(std::shared_ptr<CType> elementType, int elementCount)
    :CType(Kind::TY_Array, elementCount * elementType->GetSize(), elementType->GetAlign()), elementType(elementType), elementCount(elementCount) {}
    
    std::shared_ptr<CType> GetElementType() {
        return elementType;
    }

    const int GetElementCount() {
        return elementCount;
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
public:
    CFuncType(std::shared_ptr<CType> retType, const std::vector<Param>& params, llvm::StringRef name);
    
    const llvm::StringRef GetName() {
        return name;
    }

    const std::vector<Param> &GetParams() {
        return params;
    }

    std::shared_ptr<CType> GetRetType() {
        return retType;
    }

    llvm::Type * Accept(TypeVisitor *v) override {
        return v->VisitFuncType(this);
    }

    static bool classof(const CType *ty) {
        return ty->GetKind() == TY_Func;
    }
};