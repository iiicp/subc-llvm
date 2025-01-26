struct A {
    int x, y;
};
struct A f() {
    struct A a;
    a.x = 1;
    return a;
}
int main() {
    int ret = f().x;
    return ret;
    //struct A a = f();    
    //return a.x;
}
