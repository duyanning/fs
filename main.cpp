#include <stdio.h>
#include <string.h>
#include "fs.h"

void show_dir_entry(DirEntry* e)
{
    printf("%s\t\t%d\n", e->name, e->size);
}

int main()
{
	Disk disk;
	if (!disk.is_ok()) {
        puts("disk error");
        return 0;
	}

	FileSystem fs(&disk);

    puts("use 'help' to get available commands");
    char cmd[20];
    char filename[50];

    for (;;) {
        printf("> ");
        scanf("%s", cmd);
        if (!fs.is_formatted() && strcmp(cmd, "format") != 0 && strcmp(cmd, "quit") != 0) {
            puts("Disk has not been formatted. Use format first.");
            continue;
        }

        if (strcmp(cmd, "quit") == 0)
            break;
        else if (strcmp(cmd, "ls") == 0) {
            fs.traverse(show_dir_entry);
        }
        else if (strcmp(cmd, "format") == 0) {
            fs.format();
        }
        else if (strcmp(cmd, "rm") == 0) {
            scanf("%s", filename);
            fs.remove(filename);
        }
        else if (strcmp(cmd, "touch") == 0) {
            scanf("%s", filename);
            fs.newfile(filename);
        }
        else if (strcmp(cmd, "cat") == 0) {
            scanf("%s", filename);
            int fd = fs.open(filename);
            char buf[100];
            memset(buf, 0, sizeof buf); // 将buf清零
            fs.read(fd, buf, fs.get_size(fd));
            puts(buf);
            fs.close(fd);
        }
        else if (strcmp(cmd, "write") == 0) {
            scanf("%s", filename);
            char buf[100];
            scanf("%s", buf);
            int fd = fs.open(filename);
            fs.write(fd, buf, strlen(buf));
            fs.close(fd);
        }
        else if (strcmp(cmd, "write2") == 0) { // 在不关闭文件的情况下连写两次
            scanf("%s", filename);
            char buf1[100];
            scanf("%s", buf1);
            char buf2[100];
            scanf("%s", buf2);
            int fd = fs.open(filename);
            fs.write(fd, buf1, strlen(buf1));
            fs.write(fd, buf2, strlen(buf2));
            fs.close(fd);
        }
        else if (strcmp(cmd, "help") == 0) {
            puts("format\t\t\t\tformat the disk");
            puts("touch <filename>\t\tcreate a new file");
            puts("rm <filename>\t\t\tremove a new file");
            puts("ls\t\t\t\tlist all files");
            puts("cat <filename>\t\t\tshow a file");
            puts("write <filename> <content>\tcreate a file having the content");
        }
    }

	return 0;
}
