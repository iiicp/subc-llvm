int printf(const char *fmg, ...);
typedef char * PCHAR;
int main (void)
{
    //char * str = "学嵌入式，到宅学部落";
    PCHAR str = "学嵌入式，到宅学部落";
    printf ("str: %s\n", str);
    return 0;
}