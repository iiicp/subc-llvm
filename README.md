# 基于llvm的subc实现 

## 一：开发环境 
Ubuntu20.04 + llvm17.0 

## 二：实现的完整文法 
``` 
prog       ::= external-decl+
external-decl ::= func-def | decl-stmt
func-def   ::= decl-spec declarator block-stmt
block-stmt ::= stmt*
stmt       ::= decl-stmt | expr-stmt | null-stmt | if-stmt | block-stmt | for-stmt | break-stmt | continue-stmt
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
``` 

### 详情见测试用例 