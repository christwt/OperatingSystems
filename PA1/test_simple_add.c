#include<stdio.h>
#include<linux/kernel.h>
#include<sys/syscall.h>
#include<unistd.h>
#include<stdlib.h>

int main() {
	int number1 = 1;
	int number2 = 2;
	int *result = malloc(sizeof(int));

	syscall(327, number1, number2, result);
	return 0;
}
