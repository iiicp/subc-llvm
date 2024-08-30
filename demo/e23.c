
typedef float (*func_t)(double a, int b);

int printf(const char *fmg, ...);

float sum (double a, int b)
{
    return a + b;
} 
int main (void)
{
    func_t fp = sum;
    printf ("%f\n", fp(1,2));
    return 0;
}