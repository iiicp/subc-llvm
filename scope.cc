#include "scope.h"

Scope::Scope() {
    envs.push_back(std::make_shared<Env>());
}
void Scope::EnterScope() {
    envs.push_back(std::make_shared<Env>());
}
void Scope::ExitScope() {
    envs.pop_back();
}
std::shared_ptr<Symbol> Scope::FindObjSymbol(llvm::StringRef name) {
    for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
        auto &table = (*it)->objSymbolTable;
        if (table.count(name) > 0) {
            return table[name];
        }
    }
    return nullptr;
}
std::shared_ptr<Symbol> Scope::FindObjSymbolInCurEnv(llvm::StringRef name) {
    auto &table = envs.back()->objSymbolTable;
    if (table.count(name) > 0) {
        return table[name];
    }
    return nullptr;
}
void Scope::AddObjSymbol(std::shared_ptr<CType> ty, llvm::StringRef name) {
    auto symbol = std::make_shared<Symbol>(SymbolKind::kobj, ty, name);
    envs.back()->objSymbolTable.insert({name, symbol});
}

void Scope::AddTypedefSymbol(std::shared_ptr<CType> ty, llvm::StringRef name) {
    auto symbol = std::make_shared<Symbol>(SymbolKind::ktypedef, ty, name);
    envs.back()->objSymbolTable.insert({name, symbol});
}

std::shared_ptr<Symbol> Scope::FindTagSymbol(llvm::StringRef name) {
    for (auto it = envs.rbegin(); it != envs.rend(); ++it) {
        auto &table = (*it)->tagSymbolTable;
        if (table.count(name) > 0) {
            return table[name];
        }
    }
    return nullptr;
}

std::shared_ptr<Symbol> Scope::FindTagSymbolInCurEnv(llvm::StringRef name) {
    auto &table = envs.back()->tagSymbolTable;
    if (table.count(name) > 0) {
        return table[name];
    }
    return nullptr;
}

void Scope::AddTagSymbol(std::shared_ptr<CType> ty, llvm::StringRef name) {
    auto symbol = std::make_shared<Symbol>(SymbolKind::ktag, ty, name);
    envs.back()->tagSymbolTable.insert({name, symbol});
}