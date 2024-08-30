typedef struct {
    int a,b;
}Point;

int main() {
    Point p = {1,2};
    return p.a + p.b;
}