#include "codegen.h"
#include "diag_engine.h"
#include "lexer.h"
#include "parser.h"
#include "print_visitor.h"
#include "sema.h"

#include "llvm/CodeGen/CommandFlags.h"
#include "llvm/Support/ErrorOr.h"
#include "llvm/Support/MemoryBuffer.h"

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/CodeGen.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Target/TargetMachine.h"

#include <llvm/TargetParser/Host.h>

using namespace llvm;
static cl::opt<std::string>
InputFilename(cl::Positional, cl::desc("<input c source code>"), cl::init("-"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output Asmber"), cl::value_desc("filename"));

static cl::opt<std::string>
TargetTriple("mtriple", cl::desc("Override target triple for module"));

/// #define JIT_TEST
int main(int argc, char *argv[]) {
  cl::ParseCommandLineOptions(argc, argv, "llvm system compiler\n");

  /// 初始化后端
  llvm::InitializeAllTargets();
  llvm::InitializeAllTargetMCs();
  llvm::InitializeAllAsmPrinters();
  llvm::InitializeAllAsmParsers();

#ifdef JIT_TEST
  LLVMLinkInMCJIT();
#endif

  static llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> Buf = llvm::MemoryBuffer::getFile(InputFilename);
  if (!Buf) {
    llvm::errs() << "can't open file!!!\n";
    return -1;
  }

  llvm::SourceMgr Mgr;
  DiagEngine DiagE(Mgr);

  Mgr.AddNewSourceBuffer(std::move(*Buf), llvm::SMLoc());

  Lexer Lex(Mgr, DiagE);
  Sema SM(DiagE);
  Parser P(Lex, SM);
  auto Prog = P.ParseProgram();
  // PrintVisitor visitor(program);
  CodeGen CG(Prog);

  auto &M = CG.GetModule();

  llvm::outs() << "1>>>  output ir\n";
  M->print(llvm::outs(), nullptr);
  assert(!llvm::verifyModule(*M));

#ifdef JIT_TEST
  {
    llvm::EngineBuilder builder(std::move(M));
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
#else

  std::string CustomTriple;
  // If we are supposed to override the target triple, do so now.
  if (!TargetTriple.empty())
    CustomTriple = (Triple::normalize(TargetTriple));

  Triple T(CustomTriple);
  if (T.getTriple().empty()) {
    T.setTriple(sys::getDefaultTargetTriple());
  }
  /// 1. 初始化一个triple
  // llvm::Triple T;
  // T.setArch(llvm::Triple::ArchType::aarch64);
  // T.setOS(llvm::Triple::OSType::Darwin);
  // T.setEnvironment(llvm::Triple::GNU);
  llvm::outs() << "\nthe triple: " << T.normalize() << "\n\n";
  /// 2. 查找一个target
  std::string Err;
  const llvm::Target *TG = llvm::TargetRegistry::lookupTarget(T.normalize(), Err);
  if (!TG) {
    llvm::WithColor::error() << "target lookup failed with error: " << Err << "\n";
    return 0;
  }

  /// 3. 创建target machine
  auto Machine = std::unique_ptr<llvm::TargetMachine>(
    TG->createTargetMachine(T.normalize(), "generic", "", {}, llvm::Reloc::Model::Static, {}, llvm::CodeGenOptLevel::None));

  M->setTargetTriple(T.normalize());
  M->setDataLayout(Machine->createDataLayout());

  /// 4. 对比上面，此时输出会有 data layout信息
  llvm::outs() << "2>>>  add data layout and triple, output ir\n";
  M->print(llvm::outs(), nullptr);
  assert(!llvm::verifyModule(*M));

  /// 4. 创建输出
  std::error_code EC;
  llvm::raw_fd_ostream OS(OutputFilename, EC, llvm::sys::fs::OpenFlags::OF_None);
  if (EC)
  {
    llvm::WithColor::error()
      << "Can not open file \n";
    return {};
  }

  /// 5. 通过target machine 来串联 输入(module)和输出(汇编文件)
  llvm::legacy::PassManager PM;
  if (Machine->addPassesToEmitFile(PM, OS, nullptr,
                              CodeGenFileType::AssemblyFile)) {
    llvm::WithColor::error()
        << "No support for file type\n";
    return -1;
  }

  /// 6. 执行
  PM.run(*M);

#endif
    return 0;
}
