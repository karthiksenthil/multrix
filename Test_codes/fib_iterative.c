#include<stdio.h>

int main()
{
 int first=0,second=1,third,i,n;

 n = 5;		/* No. of elements */
 printf("\n%d %d",first,second);

 for(i=2;i<n;++i)
 {
  third=first+second;
  printf(" %d",third);
  first=second;
  second=third;
 }
  return 0;
}
