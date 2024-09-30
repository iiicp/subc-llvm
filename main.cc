#include "lexer.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/ErrorOr.h"
#include "parser.h"
#include "print_visitor.h"
#include "codegen.h"
#include "sema.h"
#include "diag_engine.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#define JIT_TEST
int main(int argc, char *argv[]) {
#ifdef JIT_TEST
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    LLVMLinkInMCJIT();
#endif

    if (argc < 2) {
        printf("please input filename!\n");
        return 0;
    }

    const char *file_name = argv[1];

    static llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buf = llvm::MemoryBuffer::getFile(file_name);
    if (!buf) {
        llvm::errs() << "can't open file!!!\n";
        return -1;
    }

    llvm::SourceMgr mgr;
    DiagEngine diagEngine(mgr);

    mgr.AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());

    Lexer lex(mgr, diagEngine);
    Sema sema(diagEngine);
    Parser parser(lex, sema);
    auto program = parser.ParseProgram();
    // PrintVisitor visitor(program);
    CodeGen codegen(program);

    auto &module = codegen.GetModule();
    module->print(llvm::outs(), nullptr);
    assert(!llvm::verifyModule(*module));
#ifdef JIT_TEST
    {
        llvm::EngineBuilder builder(std::move(module));
        std::string error;
        auto ptr = std::make_unique<llvm::SectionMemoryManager>();
        auto ref = ptr.get();
        std::unique_ptr<llvm::ExecutionEngine> ee(
            builder.setErrorStr(&error)
                    .setEngineKind(llvm::EngineKind::JIT)
                    .setOptLevel(llvm::CodeGenOptLevel::None)
                    .setSymbolResolver(std::move(ptr))
                    .create());
        ref->finalizeMemory(&error);

        void *addr = (void *)ee->getFunctionAddress("main");
        int res = ((int (*)())addr)();
        llvm::errs() << "result: " << res << "\n";
    }
#endif
    return 0;
}