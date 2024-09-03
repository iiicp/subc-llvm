int printf(const char *fmg, ...);

typedef struct student
{
    char name[20];
    int  age;
    float score;
}student_t, *student_ptr;

int main (void)
{
    student_t   stu = {"wit", 20, 99};
    student_t  *p1 = &stu;
    student_ptr p2 = &stu;
    printf ("name: %s\n", p1->name);
    printf ("name: %s\n", p2->name); 
    return 0;
}