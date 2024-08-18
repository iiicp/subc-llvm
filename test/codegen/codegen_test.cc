#include <gtest/gtest.h>

#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/SectionMemoryManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Support/TargetSelect.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

#include <stdarg.h>
#include <functional>

bool TestProgramUseJit(llvm::StringRef content, int expectValue) {
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();
    LLVMLinkInMCJIT();

    llvm::ErrorOr<std::unique_ptr<llvm::MemoryBuffer>> buf = llvm::MemoryBuffer::getMemBuffer(content, "stdin");
     if (!buf) {
        llvm::errs() << "open file failed!!!\n";
        return false;
    }
    llvm::SourceMgr mgr;
    DiagEngine diagEngine(mgr);
    mgr.AddNewSourceBuffer(std::move(*buf), llvm::SMLoc());

    Lexer lex(mgr, diagEngine);
    Sema sema(diagEngine);
    Parser parser(lex, sema);

    auto program = parser.ParseProgram(); 
    CodeGen codegen(program);
    auto &module = codegen.GetModule();
    EXPECT_FALSE(llvm::verifyModule(*module));
    {
        llvm::EngineBuilder builder(std::move(module));
        std::string error;
        auto ptr = std::make_unique<llvm::SectionMemoryManager>();
        auto ref = ptr.get();
        std::unique_ptr<llvm::ExecutionEngine> ee(
            builder.setErrorStr(&error)
                    .setEngineKind(llvm::EngineKind::JIT)
                    .setOptLevel(llvm::CodeGenOpt::None)
                    .setSymbolResolver(std::move(ptr))
                    .create());
        ref->finalizeMemory(&error);

        void *addr = (void *)ee->getFunctionAddress("main");
        int res = ((int (*)())addr)();
        if (res != expectValue) {
            llvm::errs() << "expected: " << expectValue << ", but got " << res << "\n";
        }
        EXPECT_EQ(res, expectValue);
    }
    return true;
}

TEST(CodeGenTest, assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a = b; return a;}", 5);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, add_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a += b; return a;}", 8);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, sub_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a -= b; return a;}", -2);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, mul_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a *= b; return a;}", 15);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, div_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a /= b; return a;}", 0);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, mod_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; a %= b; return a;}", 3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, bit_or_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; return a |= b;}", 7);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, bit_and_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; return a &= b;}", 1);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, bit_xor_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; return a ^= b;}", 6);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, left_shift_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; return a <<= b;}", 96);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, right_shift_assign) {
    bool res = TestProgramUseJit("int main(){int a; int b = 4; a = 3; b = 5; return a >>= b;}", 0);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, three_op1) {
    bool res = TestProgramUseJit("int main(){int a = 1, b = 2, ans; ans = (a == 1 ? (b == 2 ? 3 : 5) : 0); return ans;}", 3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, three_op2) {
    bool res = TestProgramUseJit("int main(){int a = 10, b = 20, c; c = (a < b) ? a : b; return c;}", 10);
    ASSERT_EQ(res, true);
}


TEST(CodeGenTest, sizeof_int) {
    bool res = TestProgramUseJit("int main(){int a = 10; return sizeof(int);}", 4);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, sizeof_pointer) {
    bool res = TestProgramUseJit("int main(){int a = 10; return sizeof(int*);}", 8);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, sizeof_unary) {
    bool res = TestProgramUseJit("int main(){int a = 10; return sizeof(a) + sizeof a;}", 8);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_positive) {
    bool res = TestProgramUseJit("int main(){int a = 10; return +a;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_negative) {
    bool res = TestProgramUseJit("int main(){int a = 10; return -a;}", -10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_logical_not) {
    bool res = TestProgramUseJit("int main(){int a = 10; return !a;}", 0);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_bit_not) {
    bool res = TestProgramUseJit("int main(){int a = 10; return ~a;}", -11);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_addr_dref) {
    bool res = TestProgramUseJit("int main(){int a = 10; int *p = &a; return *p;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_inc) {
    bool res = TestProgramUseJit("int main(){int a = 10; return ++a;}", 11);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_dec) {
    bool res = TestProgramUseJit("int main(){int a = 10; return --a;}", 9);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, post_dec) {
    bool res = TestProgramUseJit("int main(){int a = 10; return a--;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, post_inc) {
    bool res = TestProgramUseJit("int main(){int a = 10; return a++;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, post_inc_dref) {
    bool res = TestProgramUseJit("int main(){int a = 10, *p = &a; return *p++;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, post_dec_dref) {
    bool res = TestProgramUseJit("int main(){int a = 10, *p = &a; return  *p--;}", 10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, unary_dref_assign) {
    bool res = TestProgramUseJit("int main(){int a = 10, b = 20, *p = &a; *p = 100; return a;}", 100);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, array_subscript) {
    bool res = TestProgramUseJit("int main(){int a[3]; a[0] = 4;return a[0];}", 4);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, array_subscript2) {
    bool res = TestProgramUseJit("int main(){int a[3][5]; a[2][4] = 4;return a[2][4];}", 4);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, array_init1) {
    bool res = TestProgramUseJit("int main(){int a[3] = {1,101}; return a[1];}", 101);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, array_init2) {
    bool res = TestProgramUseJit("int main(){int a[3][4] = {{1,101},{2,6}}; return a[1][1];}", 6);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, array_init3) {
    bool res = TestProgramUseJit("int main(){int a[3][4] = {{1,101},{2,6}}; int (*p)[3][4] = &a; return (*p)[1][1];}", 6);
    ASSERT_EQ(res, true);
}

/// 

TEST(CodeGenTest, struct_1) {
    bool res = TestProgramUseJit("int main(){struct {int *p; int a,b; struct{int a;int b;} c;} a; a.c.b = 1024; a.p = &a.c.b; a.c.b += 111;return  *a.p;}", 1135);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_2) {
    bool res = TestProgramUseJit("int main(){struct {int *p; int a,b; struct A{int a;int b;} c;} a; a.c.b = 1024; a.p = &a.c.b; a.c.b += 111;return  *a.p;}", 1135);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, union_1) {
    bool res = TestProgramUseJit("int main(){struct {int *p; int a,b; union{int a;int b;} c;} a; a.c.b = 1024; a.c.a = 22; a.p = &a.c.b; a.c.b += 111; return *a.p;}", 133);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_init1) {
    bool res = TestProgramUseJit("int main(){struct {int a,b;} a = {1,2}; return a.a;}", 1);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_init2) {
    bool res = TestProgramUseJit("int main(){struct {int a,b;} a = {1,2}; return a.b;}", 2);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_init3) {
    bool res = TestProgramUseJit("int main(){struct {int a,b; struct {int d; int a;} c;} a = {1,2,{4,5}}; return a.c.d;}", 4);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_init4) {
    bool res = TestProgramUseJit("int main(){struct {int a,b; struct {int d; int a;} c;} a = {1,2,{4,5}}; return a.c.a;}", 5);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, union_init1) {
    bool res = TestProgramUseJit("int main(){union {int a,b;} a = {1}; return a.a;}", 1);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, union_init2) {
    bool res = TestProgramUseJit("int main(){union {int a,b;} a = {1}; return a.b;}", 1);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, union_init3) {
    bool res = TestProgramUseJit("int main(){union {int a,b; struct {int a,b;} c;} a = {1}; return a.c.a;}", 1);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, func1) {
    bool res = TestProgramUseJit(R"(
    struct { 
        int a, b; 
        struct {int d; int a;} c;
    } a = {1,2, {10}}; 

    int sum(int n) {
        int ret = 0;
        for (int i = 0; i <= n; ++i) {
            ret += i;
        }
        return ret;
    }

    int main() {
        return a.c.d + sum(100);
    })", 5060);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, func2) {
    bool res = TestProgramUseJit(R"(
    int a = 10;
    int main() {
        if (a != 10) {
            a = 2;
        }
        else {
            a = 20;
        }
        return a;
    }
    )", 20);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, func3) {
    bool res = TestProgramUseJit(R"(
    int main(){
        int a=0;
        int count=0;
        for(; a<=0; ){
            a=a-1;
            count=count+1;
            if(a<-20)
                break;
        }
        return count;
    }
    )", 21);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, func4) {
    bool res = TestProgramUseJit(R"(
// #include <stdio.h>
int x = 1;
int y = 2;
int z = 3;
int a = 4;
int b = 5;
int c = 6;
int d = 7;
int e = 8;
int f = 9;
int g = 10;
int h = 11;
int i = 12;
int j = 13;
int k = 14;
int l = 15;
int m = 16;
int n = 17;
int o = 18;
int p = 19;
int q = 20;

int main() {
    // Local variables
    int x1 = 1;
    int x2 = 2;
    int x3 = 3;
    int x4 = 4;
    int x5 = 5;
    int x6 = 6;
    int x7 = 7;
    int x8 = 8;
    int x9 = 9;
    int x10 = 10;
    int x11 = 11;
    int x12 = 12;
    int x13 = 13;
    int x14 = 14;
    int x15 = 15;
    int x16 = 16;
    int x17 = 17;
    int x18 = 18;
    int x19 = 19;
    int x20 = 20;

    // Perform some operations on global variables
    x = x + 1;
    y = y + 2;
    z = z + 3;
    a = a + 4;
    b = b + 5;
    c = c + 6;
    d = d + 7;
    e = e + 8;
    f = f + 9;
    g = g + 10;
    h = h + 11;
    i = i + 12;
    j = j + 13;
    k = k + 14;
    l = l + 15;
    m = m + 16;
    n = n + 17;
    o = o + 18;
    p = p + 19;
    q = q + 20;

    // Perform some operations on local variables
    x1 = x1 * 2;
    x2 = x2 * 2;
    x3 = x3 * 2;
    x4 = x4 * 2;
    x5 = x5 * 2;
    x6 = x6 * 2;
    x7 = x7 * 2;
    x8 = x8 * 2;
    x9 = x9 * 2;
    x10 = x10 * 2;
    x11 = x11 * 2;
    x12 = x12 * 2;
    x13 = x13 * 2;
    x14 = x14 * 2;
    x15 = x15 * 2;
    x16 = x16 * 2;
    x17 = x17 * 2;
    x18 = x18 * 2;
    x19 = x19 * 2;
    x20 = x20 * 2;

    int ret = x1 + x2 + x3 + x4 + x5 + x6 + x7 + x8 + x9 + x10 + x11 + x12 + x13 + x14 + x15 + x16 + x17 + x18 + x19 + x20
    + x + y + z + a + b + c + d + e + f + g + h + i + j + k + l + m + n + o + p + q;
    // printf("ret = %d\n", ret);
    return ret;
}
    )", 840);
    ASSERT_EQ(res, true);
}