#include<stdio.h>
void sum(int x,int y)
{
  int s;
  s=x+y;
}
void main()
{
  int a,b;
  b=0;
   for(a=0;a<10;a++)
        b = b+1;
	sum(a,b);
printf("%d\n",b);

}
