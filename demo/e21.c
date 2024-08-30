int printf(const char *fmg, ...);

typedef int array_t[10]; 
array_t array;
int main (void)
{
    array[9] = 100;
    printf ("array[9] = %d\n", array[9]);
    return 0;
}