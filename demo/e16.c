int printf(const char *fmg, ...);

int main ()
{
   /* 局部变量定义 */
   char grade = 'B';
   int ret = grade;
   switch(grade)
   {
   case 'A' : {
      printf("很棒！\n" );
      break;
   }
   case 'B' : 
      // printf("ddd\n");
      // break;
   case 'C' : 
      ret += 1;
      printf("做得好\n" );
      break;
   case 'D' : {
      printf("您通过了\n" );
      break;
   }
   case 'F' : {
      printf("最好再试一下\n" );
      break;
   }
   default : {
      printf("无效的成绩\n" );
      break;
   }
   }
   printf("您的成绩是 %c\n", grade );
 
   return ret;
}
