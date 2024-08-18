#pragma once
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/FormatVariadic.h"

namespace diag{
enum {
    #define DIAG(ID, KIND, MSG) ID,
    #include "diag.inc"
};
};

class DiagEngine {
private:
    llvm::SourceMgr &mgr;
private:
    llvm::SourceMgr::DiagKind GetDiagKind(unsigned id);
    const char *GetDiagMsg(unsigned id);
public:
    DiagEngine(llvm::SourceMgr &mgr) : mgr(mgr) {}

    template <typename... Args>
    void Report(llvm::SMLoc loc, unsigned diagId, Args... args) {
        auto kind = GetDiagKind(diagId);
        const char *fmt = GetDiagMsg(diagId);
        auto f = llvm::formatv(fmt, std::forward<Args>(args)...).str();
        mgr.PrintMessage(loc, kind, f);

        if (kind == llvm::SourceMgr::DK_Error) {
            exit(0);
        }
    }
};