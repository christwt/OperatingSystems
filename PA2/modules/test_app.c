#include <stdio.h>		// std i/o, required for functions such as scanf().
#include <stdlib.h>		//
#include <fcntl.h>		// for file access modes for open(), close()

#define BUFFER_SIZE 1024

int main() {
	char option, buffer[BUFFER_SIZE];
	/* 
	open file, make sure to have run --sudo mknod -m 777 /dev/simple_character_device c 247 0
	to create the file to work with our character device. 
	utilize O_RDWR option which stands for read, write permissions.
	*/
	int file = open("/dev/simple_character_device", O_RDWR);

	// Opening header.
	printf("Welcome to test_app\n");

	while(1) {
		// Options menu.
		printf("\n\nOptions:\nPress r to read from the device\nPress w to write to the device\nPress e to exit from the device\nPress anything else to keep reading or writing from the device\n\nEnter option: ");
		scanf("%c", &option);
		switch (option) {
			case 'r':
			case 'R':
				/*read case
				read() function attempts to read BUFFER_SIZE bytes from file, and places the characters read into buffer.
				*/				
				read(file, buffer, BUFFER_SIZE);
				printf("Device output: %s\n", buffer);
				while (getchar() != '\n');
				break;
			case 'w':
			case 'W':
				/* write case
				write() function attempts to write BUFFER_SIZE bytes from buffer to file.
				*/
				printf("Enter data to write to the device: ");
				scanf("%s", buffer);
				write(file, buffer, BUFFER_SIZE);
				while (getchar() != '\n');
				break;
				// exit case
			case 'e':
			case 'E':
				printf("Exiting test_app\n\nGoodbye\n");
				return 0;
			default: 
				while (getchar() != '\n');
		}
	}
	// close file on exit. 
	close(file);
	return 0;
}
