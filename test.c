#include <stdio.h>

int main(int argc, const char *argv[])
{
	int a;
	if (scanf("%d",&a) == EOF){
		printf("输入错误\n");
	};
	printf("%d",a);
	return 0;
}
