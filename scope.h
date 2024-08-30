#pragma once
#include "llvm/ADT/StringMap.h"
#include "type.h"
#include <memory>

enum class SymbolKind {
    kobj, /// var, func
    ktypedef, // typedef
    ktag  /// struct/union
};

class Symbol {
private:
    SymbolKind kind;
    std::shared_ptr<CType> ty;
    llvm::StringRef name;
public:
    Symbol(SymbolKind kind, std::shared_ptr<CType> ty, llvm::StringRef name):kind(kind), ty(ty), name(name) {}
    std::shared_ptr<CType> GetTy() {return ty;}
    SymbolKind GetKind() {return kind;}
};


class Env{
public:
    /// 符号信息
    llvm::StringMap<std::shared_ptr<Symbol>> objSymbolTable;
    llvm::StringMap<std::shared_ptr<Symbol>> tagSymbolTable;
};

class Scope {
private:
    std::vector<std::shared_ptr<Env>> envs;
public:
    Scope();
    void EnterScope();
    void ExitScope();
    std::shared_ptr<Symbol> FindObjSymbol(llvm::StringRef name);
    std::shared_ptr<Symbol> FindObjSymbolInCurEnv(llvm::StringRef name);
    void AddObjSymbol(std::shared_ptr<CType> ty, llvm::StringRef name);
    void AddTypedefSymbol(std::shared_ptr<CType> ty, llvm::StringRef name);

    std::shared_ptr<Symbol> FindTagSymbol(llvm::StringRef name);
    std::shared_ptr<Symbol> FindTagSymbolInCurEnv(llvm::StringRef name);
    void AddTagSymbol(std::shared_ptr<CType> ty, llvm::StringRef name);
};