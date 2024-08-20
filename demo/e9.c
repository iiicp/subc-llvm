struct A {
    int a;
    int b;
    struct A *c;
};


struct A getNewStruct(struct A a) {
    struct A newA;
    newA.a = a.a + 102; // 103
    newA.b = a.b + 1024; // 1026
    newA.c = 0;
    return newA;
}

int main() {
    struct A a = {1,2,0};
    struct A newA = getNewStruct(a);
    return newA.a + newA.b;
}