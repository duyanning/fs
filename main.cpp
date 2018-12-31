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
        if (!fs.is_formatted() &&strcmp(cmd, "format") != 0) {
            puts("Disk has not been formatted. Use format first.");
        }

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
            fs.remove(filename);
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
        if (strcmp(cmd, "help") == 0) {
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
