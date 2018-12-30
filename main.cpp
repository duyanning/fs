#include <stdio.h>
#include <string.h>
#include "fs.h"

void show_dir_entry(DirEntry* e)
{
    printf("%s\t\t%d\n", e->name, e->size);
}

int main()
{
    // 确保你的修改不会导致数据结构的大小不等于一扇区
    assert(sizeof(Sector0) == SECTOR_SIZE);
    assert(sizeof(Sector1) == SECTOR_SIZE);

	Disk disk;
	if (!disk.is_ok()) {
        puts("disk error");
        return 0;
	}

	FileSystem fs(&disk);
	if (!fs.is_formatted()) {
        puts("disk has not been formatted. Use format.");
	}


    char cmd[20];
    char filename[50];

    for (;;) {
        printf("> ");
        scanf("%s", cmd);

        if (strcmp(cmd, "quit") == 0)
            break;
        if (strcmp(cmd, "ls") == 0) {
            fs.traverse(show_dir_entry);
        }
        if (strcmp(cmd, "format") == 0) {
            fs.format();
        }
        if (strcmp(cmd, "rm") == 0) {
            scanf("%s", filename);
            puts(filename);
        }
        if (strcmp(cmd, "touch") == 0) {
            scanf("%s", filename);
            fs.newfile(filename);
        }
        if (strcmp(cmd, "cat") == 0) {
            scanf("%s", filename);
            int fd = fs.open(filename);
            char buf[100];
            memset(buf, 0, sizeof buf); // 把缓冲区全清零
            fs.read(fd, buf, fs.get_size(fd));
            puts(buf);
        }
        if (strcmp(cmd, "write") == 0) {
            scanf("%s", filename);
            char buf[100];
            scanf("%s", buf);
            int fd = fs.open(filename);
            fs.write(fd, buf, strlen(buf));
            fs.close(fd);
        }


    }


//	printf("%d", SECTOR_SIZE / sizeof(DirEntry));
//	char sector[SECTOR_SIZE] = "abc";
//	disk.write(1, sector);
//	char sector2[SECTOR_SIZE];
//	disk.read(1, sector2);
//	puts(sector2);

//	fs.newfile("a.txt");
//	int fd = fs.open("a.txt");
//	char buf[] = "abcdefghij";
//	fs.write(fd, buf, sizeof buf);
//	fs.close(fd);
////
//	fd = fs.open("a.txt");
//	char buf2[100];
//	fs.read(fd, buf2, 11);
//	puts(buf2);

	return 0;
}
