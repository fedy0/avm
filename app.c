#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#define DEVICE_NAME "/dev/avm"
#define BUFFER_SIZE 150


int main () {
    char write_data[BUFFER_SIZE];
    char read_data[BUFFER_SIZE];
    int i, fd;
    char c;
    
    fd = open(DEVICE_NAME, O_RDWR);
    if (fd == -1) {
        printf("Error: Could not open device\n");
        exit(-1);
    }
    
    printf("'r' for read, 'w' for write, 'q' to quit \n Enter command: ");
    while (1) {    
        scanf("%c", &c);
        getchar();
        switch (c) {
           case 'w' :
               printf("Write to device: ");
               fgets(write_data, sizeof(write_data), stdin);
               write_data[strcspn(write_data, "\n")] = 0;
               write(fd, write_data, sizeof(write_data));
               printf("\nEnter command: ");
           break;
           case 'r':
               read(fd, read_data, sizeof(read_data));
               printf("Read from device: %s", read_data);
               printf("\nEnter command: ");
           break;
           case 'q':
               close(fd);
               exit(0);
           break;
           default:
               printf("Choose 'r' or 'w' to read or write into the device and 'q' to quit the app\n Enter command:");
           break;
        }
    }
    close(fd);
    return 0;
}
