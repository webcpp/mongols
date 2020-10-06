#include <studio.h>
int main()
{
   int a,b;
   a=20;
   b=50;


   printf("before swaping \n");
   printf("a= %d \n", a);
   printf("b= %d \n, b);
   
   a=a+b; //a=70 (20+50)
   b=a-b; //b=20 (70-50)
   a=a-b; //a=50 (70-20)
   
   printf("after swaping \n");
   printf("a= %d \n", a);
   printf("b= %d \n, b);


   return 0;
}
