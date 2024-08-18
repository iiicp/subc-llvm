#include "diag_engine.h"
using namespace llvm;
static const char *diag_msg[] = {
#define DIAG(ID, KIND, MSG) MSG,
#include "diag.inc"
};

static llvm::SourceMgr::DiagKind diag_kind[] = {
#define DIAG(ID, KIND, MSG) SourceMgr::DK_##KIND, 
#include "diag.inc"
};

llvm::SourceMgr::DiagKind DiagEngine::GetDiagKind(unsigned id) {
    return diag_kind[id];
}

const char *DiagEngine::GetDiagMsg(unsigned id) {
    return diag_msg[id];
}