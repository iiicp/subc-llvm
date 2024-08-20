int main(){
    int aa = 1, b = 1;
    aa = aa || b && aa || b || aa || b && aa ;
    aa = aa << 3;
    return aa+b;
}

