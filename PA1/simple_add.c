#include<linux/kernel.h>
#include<linux/linkage.h>
asmlinkage    long     sys_simple_add(int number1, int number2, int *result){
	*result = number1 + number2;
	printk(KERN_EMERG "Result of simple_add syscall: %d\n", *result);
	return 0;
}
