#include<stdio.h>

int a;

void sum()
{
	int x =8;
	int y = x+1;
	printf("%d\n",y);
}

void add(int b)
{
	a = 42;
	b = a + 1;
	printf("%d\n",b);
	sum();
}

void main()
{
	int b;
	a = 10;
	a = 11;
	b = a+1;
	printf("%d\n",b);
	add(b);

}
	
