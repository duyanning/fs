//#include <iostream>
#include <stdio.h>
#include "fs.h"


int main()
{
//	printf("%d", SECTOR_SIZE / sizeof(DirEntry));
	Disk disk;
//	char sector[SECTOR_SIZE] = "abc";
//	disk.write(1, sector);
//	char sector2[SECTOR_SIZE];
//	disk.read(1, sector2);
//	puts(sector2);
	
	FileSystem fs(&disk);
	fs.newfile("a.txt");
	int fd = fs.open("a.txt");
	char buf[] = "abcdefghij";
	fs.write(fd, buf, sizeof buf);
	fs.close(fd);
//	
	fd = fs.open("a.txt");
	char buf2[100];
	fs.read(fd, buf2, 11);
	puts(buf2);
	
	return 0;
}
