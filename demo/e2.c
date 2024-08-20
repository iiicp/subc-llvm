//int printf(const char *fmg, ...);

struct {
    int a, b; 
    struct {int d; int a;} c;
} a = {1,2,{4,5}}; 

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
//{struct {int *p; int a,b; union{int a;int b;} c;} a; a.c.b = 1024; a.c.a = 22; a.p = &a.c.b; a.c.b += 111; *a.p;}
//{struct {int *p; int a,b; union{int a;int b;} c;} a; a.c.b = 1024; a.c.a = 22; a.p = &a.c.b; a.c.b += 111; *a.p;}
//{struct {int *p; int a,b; struct{int a;int b;} c;} a; a.c.b = 1024; a.c.b;}