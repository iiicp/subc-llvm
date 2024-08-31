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

bool TestProgramCheckModule(llvm::StringRef content) {
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

TEST(CodeGenTest, while_test) {
    bool res = TestProgramUseJit(R"(
    int main() {
        int a = 10;
        while (a < 100) {
            break;
            if ( a == 35 ) 
                break;
            a += 3;
            break;
        }

        return a;
    }
    )",10);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, switch_test) {
    bool res = TestProgramUseJit(R"(
int main ()
{
   /* 局部变量定义 */
   char grade = 'B';
   int ret = grade;
   switch(grade)
   {
   case 'A' : {
      break;
   }
   case 'B' : 
      // printf("ddd\n");
      // break;
   case 'C' : {
      ret += 1;
      break;
   }
   case 'D' : 
      break;
   case 'F' : {
      break;
   }
   default : {
      break;
   }
   }
   return ret;
}
    )",67);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, for_empty) {
    bool res = TestProgramCheckModule(R"(
    int main() {
        int a = 10;
        for (;;) 
            ;
        return a;
    }
    )");
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, list) {
    bool res = TestProgramUseJit(R"(
    struct Point
{
    struct Point* next;
};

int getListCount(struct Point* first)
{
    struct Point* current = first;
    int i = 0;
    for(;current;)
    {
        current = current->next;
        i++;
    }
    return i;
}

int main()
{
    struct Point first = {0};
    struct Point second = {0};
    struct Point third = {0};
    first.next = &second;
    second.next = &third;
    third.next = 0;
    return getListCount(&first);
}
    )", 3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, list2) {
    bool res = TestProgramUseJit(R"(
    struct A {
        // int a;
        struct A *a;
    } a = {0};

    int main() {
        return 0;
    }
    )", 0);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, struct_pram) {
    bool res = TestProgramUseJit(R"(
        struct A {
            int a;
            int b;
            struct A *c;
        };


        struct A getNewStruct(struct A a) {
            struct A newA;
            newA.a = a.a + 102; //   103
            newA.b = a.b + 1024; // 1026
            newA.c = 0;
            return newA;
        }

        int main() {
            struct A a = {1,2,0};
            struct A newA = getNewStruct(a);
            return newA.a + newA.b;
        }
    )", 1129);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, arr_param) {
    bool res = TestProgramUseJit(R"(
        int sort_arr[5];
        int combine(int arr1[], int arr1_length, int arr2[], int arr2_length) {
            int i = 0;
            int j = 0;
            int k = 0;
            for (;i < arr1_length && j < arr2_length;) {
                if (arr1[i] < arr2[j]) {
                    sort_arr[k] = arr1[i];
                    i = i + 1;
                }
                else {
                    sort_arr[k] = arr2[j];
                    j = j + 1;
                }
                k = k + 1;
            }
            if (i == arr1_length) {
                for (;j < arr2_length;) {
                    sort_arr[k] = arr2[j];
                    k = k + 1;
                    j = j + 1;
                }
            }
            else {
                for (;i < arr1_length;) {
                    sort_arr[k] = arr2[i];
                    k = k + 1;
                    i = i + 1;
                }
            }
            return sort_arr[arr1_length + arr2_length - 1];
        }

        int main() {
            int a[] = { 1,5 };
            int b[] = { 1,4,14 };
            return combine(a, 2, b, 3);
        }

    )", 14);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, arr_imcomplete) {
    bool res = TestProgramUseJit(R"(
        int a[] = {1,2,3,4,5};
        int main() {
            return sizeof(a);
        }
    )", 20);
}

TEST(CodeGenTest, swap) {
    bool res = TestProgramUseJit(R"(
        void swap(int *p, int *q) {
            int t = *p;
            *p = *q;
            *q = t;
        }

        int main() {

            int a = 3, b = 6;
            swap(&a, &b);

            return a;
        }
    )", 6);
}

TEST(CodeGenTest, arr_two_dim) {
    bool res = TestProgramUseJit(R"(
        int sum(int array[ ][4],int m,int n);//该函数完成对array数组中的前m行和n列元素求和

        int main()
        {
            //定义二维数组的同时进行初始化
            int a[4][4]= {{1,2,3,4},{5,6,7,8},{9,10,11,12},{13,14,15,16}}; 
            int row = 3, col = 2;
            //输出二维数组前row行前col列的元素的和
            return sum(a, row, col); 
        }

        int sum(int array[][4], int m, int n) {
            int s = 0;
            int i, j;
            for (i = 0; i < m; ++i) {
                for (j = 0; j < n; ++j) {
                    s += array[i][j];
                }
            }
            return s;
        }
    )", 33);
}

TEST(CodeGenTest, printf1) {
    bool res = TestProgramCheckModule(R"(
        int printf(const char *fmg, ...);

        const char *hello = "hello,world\n";

        int main() {
            printf("%s\n", hello);
            return 0;
        }
    )");
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, doWhile) {
    bool res = TestProgramUseJit(R"(
        int main() {
        int a = 10;
        do {
            if ( a == 43 ) 
                break;
            a += 3;
        }while (a < 100);
        return a;
        }
    )",43);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, nqueen) {
    bool res= TestProgramCheckModule(R"(
    // How to run:
    //
    //   $ ./9cc examples/nqueen.c > tmp-nqueen.s
    //   $ gcc -static -o tmp-nqueen tmp-nqueen.s
    //   $ ./tmp-nqueen 

    int printf(const char *fmg, ...);

    void print_board(int board[][10]) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++)
        if (board[i][j])
        printf("Q ");
        else
        printf(". ");
        printf("\n");
    }
    printf("\n\n");
    }

    int conflict(int board[][10], int row, int col) {
    for (int i = 0; i < row; i++) {
        if (board[i][col])
        return 1;
        int j = row - i;
        if (0 < col - j + 1 && board[i][col - j])
        return 1;
        if (col + j < 10 && board[i][col + j])
        return 1;
    }
    return 0;
    }

    void solve(int board[][10], int row) {
    if (row > 9) {
        print_board(board);
    }
    for (int i = 0; i < 10; i++) {
        if (conflict(board, row, i)) {
        } else {
        board[row][i] = 1;
        solve(board, row + 1);
        board[row][i] = 0;
        }
    }
    }

    int main() {
    int board[100];
    for (int i = 0; i < 100; i++)
        board[i] = 0;
    solve(board, 0);
    return 0;
    }

    )");
     ASSERT_EQ(res, true);
}

TEST(CodeGenTest, 2048) {
    bool res= TestProgramCheckModule(R"(
        int printf(const char *fmg, ...);
        int scanf(const char *format, ...);
        int getchar();
        int putchar(int ch);

        int getint() {
        int val;
        scanf("%d", &val);
        return val;
        }

        int getch() { return getchar(); }

        int getarray(int val[]) {
        int len;
        for (int i = 0; i < len; i++) {
            scanf("%d", val[i]);
        }
        return len;
        }

        void putint(int val) { printf("%d", val); }

        void putch(int val) { putchar(val); }

        void putarray(int len, int arr[]) {
        printf("%d:", len);
        for (int i = 0; i < len; i++) {
            printf(" %d", arr[i]);
        }
        }

        // Constants.
        const int UP = 0;
        const int DOWN = 1;
        const int LEFT = 2;
        const int RIGHT = 3;

        const int MAP_LEN = 4;

        const int POW2[20] = {1,     2,     4,     8,      16,     32,    64,
                            128,   256,   512,   1024,   2048,   4096,  8192,
                            16384, 32768, 65536, 131072, 262144, 524288};
        const int LEN_OF_POW2[20] = {0, 1, 1, 1, 2, 2, 2, 3, 3, 3,
                                    4, 4, 4, 4, 5, 5, 5, 6, 6, 6}; // x * log10(2)

        int STR_INIT[25] = {73, 110, 112, 117, 116, 32, 97,  32,  114,
                            97, 110, 100, 111, 109, 32, 110, 117, 109,
                            98, 101, 114, 58,  32,  10, 0};
        int STR_HELP[62] = {119, 44,  32,  97,  44,  32,  115, 44,  32,  100, 58,
                            32,  109, 111, 118, 101, 10,  104, 58,  32,  112, 114,
                            105, 110, 116, 32,  116, 104, 105, 115, 32,  104, 101,
                            108, 112, 10,  113, 58,  32,  113, 117, 105, 116, 10,
                            112, 58,  32,  112, 114, 105, 110, 116, 32,  116, 104,
                            101, 32,  109, 97,  112, 10,  0};
        // "score: "
        int STR_SCORE[8] = {115, 99, 111, 114, 101, 58, 32, 0};
        // "step: "
        int STR_STEP[7] = {115, 116, 101, 112, 58, 32, 0};
        int STR_GG[12] = {71, 97, 109, 101, 32, 111, 118, 101, 114, 46, 10, 0};
        int STR_INVALID[26] = {73,  110, 118, 97,  108, 105, 100, 32,  105,
                            110, 112, 117, 116, 46,  32,  84,  114, 121,
                            32,  97,  103, 97,  105, 110, 46,  0};
        const int CHR_SPACE = 32;
        const int CHR_ENTER = 10;

        // Map, stores log2(x) if x != 0.
        int map[4][4];
        int score;
        int step;
        int max_num_len;
        int alive;

        // Random lib.
        int seed;

        int rand() {
        seed = (seed * 214013 + 2531011) % 0x40000000;
        if (seed < 0)
            seed = -seed;
        return seed / 65536 % 0x8000;
        }

        // 0 to end a string.
        void put_string(int str[]) {
        int i = 0;
        for (;str[i] != 0;) {
            putch(str[i]);
            i = i + 1;
        }
        }

        // Clears the map.
        void clear_map() {
        int x = 0, y;
        for (;x < MAP_LEN;) {
            y = 0;
            for (;y < MAP_LEN;) {
            map[x][y] = 0;
            y = y + 1;
            }
            x = x + 1;
        }
        }

        // Game init.
        void init() {
        clear_map();
        score = 0;
        step = 0;
        max_num_len = 1;
        alive = 1;
        }

        void print_map() {
        putch(CHR_ENTER);
        put_string(STR_STEP);
        putint(step);
        putch(CHR_ENTER);
        put_string(STR_SCORE);
        putint(score);
        putch(CHR_ENTER);
        int x = 0, y;
        for (;x < MAP_LEN;) {
            y = 0;
            for (;y < MAP_LEN;) {
            if (map[x][y] == 0) {
                int i = LEN_OF_POW2[map[x][y]] + 1;
                for (;i <= max_num_len;) {
                putch(95);
                i = i + 1;
                }
                putch(CHR_SPACE);
            } else {
                putint(POW2[map[x][y]]);
                int i = LEN_OF_POW2[map[x][y]];
                for (;i <= max_num_len;) {
                putch(CHR_SPACE);
                i = i + 1;
                }
            }
            y = y + 1;
            }
            putch(CHR_ENTER);
            x = x + 1;
        }
        }

        // return bool
        // var == dx or var == dy
        int move_base(int inc, int var[], int other[], int x[], int y[],
                    int is_dry_run) {
        int start, end;
        int moved = 0;
        if (inc == -1) {
            start = MAP_LEN - 1;
            end = -1;
        } else {
            start = 0;
            end = MAP_LEN;
        }
        other[0] = start;
        for(;other[0] != end;) {
            int ptr_from = start + inc, ptr_to = start;
            for(;ptr_from != end;) {
            if (ptr_from == ptr_to) {
                ptr_from = ptr_from + inc;
                continue;
            }
            var[0] = ptr_from;
            int from_num = map[x[0]][y[0]];
            var[0] = ptr_to;
            int to_num = map[x[0]][y[0]];
            if (to_num == 0) {
                if (from_num == 0) {
                ptr_from = ptr_from + inc;
                } else {
                if (is_dry_run) {
                    return 1;
                }
                map[x[0]][y[0]] = from_num;
                var[0] = ptr_from;
                map[x[0]][y[0]] = 0;
                moved = 1;
                ptr_from = ptr_from + inc;
                }
            } else {
                if (from_num == to_num) {
                if (is_dry_run) {
                    return 1;
                }
                // Merges two numbers.
                var[0] = ptr_to;
                map[x[0]][y[0]] = to_num + 1;
                var[0] = ptr_from;
                map[x[0]][y[0]] = 0;
                int new_num_len = LEN_OF_POW2[to_num + 1];
                if (new_num_len > max_num_len) {
                    max_num_len = new_num_len;
                }
                score = score + POW2[to_num + 1];
                moved = 1;
                ptr_to = ptr_to + inc;
                } else if (from_num == 0) {
                ptr_from = ptr_from + inc;
                } else {
                ptr_to = ptr_to + inc;
                }
            }
            }
            other[0] = other[0] + inc;
        }
        return moved;
        }

        void generate() {
        int x = 0, y, chosen_x, chosen_y, empty = 0;
        for(;x < MAP_LEN;) {
            y = 0;
            for(;y < MAP_LEN;) {
            if (map[x][y] == 0) {
                empty = empty + 1;
                if (rand() % empty == 0) {
                chosen_x = x;
                chosen_y = y;
                }
            }
            y = y + 1;
            }
            x = x + 1;
        }
        int num;
        if (rand() % 2 < 1) {
            num = 1;
        } else {
            num = 2;
        }
        map[chosen_x][chosen_y] = num;
        }

        void move(int pos) {
        int x[1], y[1], inc = 1 - pos % 2 * 2;
        int moved;
        if (pos / 2 == 0) {
            moved = move_base(inc, x, y, x, y, 0);
        } else {
            moved = move_base(inc, y, x, x, y, 0);
        }
        if (!moved) {
            put_string(STR_INVALID);
            putch(CHR_ENTER);
            return;
        }
        step = step + 1;
        generate();
        print_map();
        }

        int try_move() {
        int x[1], y[1];
        return move_base(1, x, y, x, y, 1) || move_base(1, y, x, x, y, 1) ||
                move_base(-1, x, y, x, y, 1) || move_base(-1, y, x, x, y, 1);
        }

        int main() {
        put_string(STR_INIT);
        seed = getint();
        init();
        generate();
        print_map();
        for(;alive;) {
            int ch = getch();
            if (ch == 119) {
            move(UP);
            } else if (ch == 97) {
            move(LEFT);
            } else if (ch == 115) {
            move(DOWN);
            } else if (ch == 100) {
            move(RIGHT);
            } else if (ch == 104) {
            // help
            put_string(STR_HELP);
            } else if (ch == 113 || ch == -1) {
            // quit
            put_string(STR_GG);
            return 0;
            } else if (ch == 112) {
            // print the map
            putch(CHR_ENTER);
            print_map();
            } else if (ch == 32 || ch == 10 || ch == 13) {
            // ' ' or '\n' or '\r'
            continue;
            } else {
            put_string(STR_INVALID);
            putch(CHR_ENTER);
            seed = (seed + ch) % 0x40000000;
            }

            if (!try_move()) {
            put_string(STR_GG);
            break;
            }
        }
        return 0;
        }
    )");
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, typedef1) {
    bool res = TestProgramUseJit(R"(
            typedef struct {
                int a,b;
            }Point;

            int main() {
                Point p = {1,2};
                return p.a + p.b;
            }
    )",3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, typedef_struct) {
    bool res = TestProgramCheckModule(R"(
        int printf(const char *fmg, ...);

        typedef struct student
        {
            char name[20];
            int  age;
            float score;
        }student_t, *student_ptr;
        int main (void)
        {
            student_t   stu = {"wit", 20, 99};
            student_t  *p1 = &stu;
            student_ptr p2 = &stu;
            printf ("name: %s\n", p1->name);
            printf ("name: %s\n", p2->name); 
            return 0;
        }
    )");
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, c1) {
    bool res = TestProgramUseJit(R"(
        typedef int (*func_t)(int a, int b);

        int printf(const char *fmg, ...);

        int sum (int a, int b)
        {
            return a + b;
        } 
        int main (void)
        {
            func_t fp = sum;
            printf ("%d\n", fp(1,2));
            return fp(1,2);
        }
    )",3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, cast_1) {
    bool res = TestProgramUseJit(R"(
        int printf(const char *fmg, ...);
        
        int main()
        {
        int  i = 17;
        char c = 'c'; /* ascii 值是 99 */
        int sum;
        
        sum = i + c;
        printf("Value of sum : %d\n", sum);
        return sum;
        }
    )", 116);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, cast_2) {
    bool res = TestProgramUseJit(R"(
        int printf(const char *fmg, ...);

        int main()
        {
        int sum = 17, count = 5;
        double mean;
        
        mean = (double) sum / count;
        printf("Value of mean : %f\n", mean );
        return mean;
        }
    )", 3);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, typedef_array) {
    bool res = TestProgramUseJit(R"(
        int printf(const char *fmg, ...);

        typedef int array_t[10]; 
        array_t array;
        int main (void)
        {
            array[9] = 100;
            printf ("array[9] = %d\n", array[9]);
            return array[9];
        }
    )", 100);
    ASSERT_EQ(res, true);
}

TEST(CodeGenTest, typedef_point) {
    bool res = TestProgramCheckModule(R"(
        int printf(const char *fmg, ...);
        typedef char * PCHAR;
        int main (void)
        {
            //char * str = "学嵌入式，到宅学部落";
            PCHAR str = "学嵌入式，到宅学部落";
            printf ("str: %s\n", str);
            return 0;
        }
    )");
    ASSERT_EQ(res, true);
}