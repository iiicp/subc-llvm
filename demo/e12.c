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

