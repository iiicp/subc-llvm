# 基于llvm的subc实现 

## 零：B站课程链接
[LLVM前端实践之从0到1实现一个C编译器](https://www.bilibili.com/cheese/play/ss31453?csource=private_space_class_null&spm_id_from=333.999.0.0)

## 一：开发环境 
Ubuntu20.04 + llvm20

构建步骤
```
git clone git@github.com:iiicp/subc-llvm.git
cd subc-llvm 
mkdir build && cd build 
cmake .. -GNinja -DLLVM_DIR=`llvm install home`/lib/cmake/llvm 
ninja 
```

目前在下列环境下编译通过：
* Ubuntu 20.04 + llvm 20
* Ubuntu 22.04 + llvm 20
* MacOS 14 + llvm 20

## 二：实现的完整文法 
``` 
prog       ::= external-decl+
external-decl ::= func-def | decl-stmt
func-def   ::= decl-spec declarator block-stmt
block-stmt ::= stmt*
stmt       ::= decl-stmt | expr-stmt | null-stmt | if-stmt | block-stmt | for-stmt | 
				break-stmt | continue-stmt | while-stmt | do-while-stm | switch-stmt |
				case-stmt | default-stmt
decl-stmt  ::= decl-spec init-declarator-list? ";"
init-declarator-list ::= declarator (= initializer)? ("," declarator (= initializer)?)*
decl-spec  ::= "int" | struct-or-union-specifier
struct-or-union-specifier ::= struct-or-union identifier "{" (decl-spec declarator(, declarator)* ";")* "}"
														  struct-or-union identifier
struct-or-union := "struct" | "union"

declarator ::= "*"* direct-declarator
direct-declarator ::= identifier | direct-declarator "[" assign "]" 
											| direct-declarator "(" parameter-type-list? ")"
parameter-type-list ::= decl-spec declarator (, decl-spec declarator)* (", " "...")?

initializer ::= assign | "{" initializer ("," initializer)*  "}"

null-stmt     ::= ";"
if-stmt       ::= "if" "(" expr ")" stmt ( "else" stmt )?
for-stmt      ::= "for" "(" expr? ";" expr? ";" expr? ")" stmt
							    "for" "(" decl-stmt expr? ";" expr? ")" stmt 
while-stmt 	  ::= "while" "(" expr ")" stmt								 
do-while-stmt ::= "do" stmt	"while" "(" expr ")" ";"
switch-stmt   ::= "switch" "(" expr ")" stmt
case-stmt	  ::= "case" expr ":" stmt 
default-stmt  ::= "default" ":" stmt
block-stmt    ::= "{" stmt* "}"
break-stmt    ::= "break" ";"
continue-stmt ::= "continue" ";"
expr-stmt     ::= expr ";"

expr         ::= assign (, assign )*
assign ::= conditional ("="|"+="|"-="|"*="|"/="|"%="|"<<="|">>="|"&="|"^="|"|=" asign)?
conditional ::= logor  ("?" expr ":" conditional)?
logor       ::= logand ("||" logand)*
logand      ::= bitor  ("&&" bitor)*
bitor       ::= bitxor ("|" bitxor)*
bitxor      ::= bitand ("^" bitand)*
bitand      ::= equal ("&" equal)*
equal       ::= relational ("==" | "!=" relational)*
relational  ::= shift ("<"|">"|"<="|">=" shift)*
shift       ::= add ("<<" | ">>" add)*
add         ::= mult ("+" | "-" mult)* 
mult        ::= cast ("*" | "/" | "%" cast)* 
cast        ::= unary | "(" type-name ")" cast
unary       ::= postfix | ("++"|"--"|"&"|"*"|"+"|"-"|"~"|"!"|"sizeof") unary
                | "sizeof" "(" type-name ")"
postfix     ::= primary | postfix ("++"|"--") | postfix "[" expr "]"
								| postfix "." identifier
								| postfix "->" identifier 
								| postfix "(" arg-expr-list? ")"
arg-expr-list := assign ("," assign)*
primary     ::= identifier | number | "(" expr ")" 
number      ::= ([0-9])+ 
identifier  ::= (a-zA-Z_)(a-zA-Z0-9_)*
type-name   ::= decl-spec abstract-declarator?
abstract-declarator ::= "*"* direct-abstract-declarator?
direct-abstract-declarator ::=  direct-abstract-declarator? "[" assign "]"
```  

## 三：测试代码 

### demo1
``` 
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
}
```  

### demo2 
``` 
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
``` 

### 详情见测试用例 
