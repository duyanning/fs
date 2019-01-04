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
    FileSystem fs(&disk);

    if (!fs.is_formatted()) {
        puts("磁盘尚未格式化，请先执行format命令格式化磁盘。");
    }

    puts("执行help命令可列出所有命令；执行quit命令退出。");
    char cmd[20];
    char filename[50];

    for (;;) {
        printf("> ");
        scanf("%s", cmd);
        if (!fs.is_formatted() && strcmp(cmd, "format") != 0 && strcmp(cmd, "quit") != 0) {
            puts("磁盘尚未格式化，请先执行format命令格式化磁盘。");
            continue;
        }

        if (strcmp(cmd, "quit") == 0)
            break;
        else if (strcmp(cmd, "ls") == 0) {
            fs.traverse(show_dir_entry);
        } else if (strcmp(cmd, "format") == 0) {
            fs.format();
        } else if (strcmp(cmd, "rm") == 0) {
            scanf("%s", filename);
            fs.remove(filename);
        } else if (strcmp(cmd, "touch") == 0) {
            scanf("%s", filename);
            fs.newfile(filename);
        } else if (strcmp(cmd, "cat") == 0) {
            scanf("%s", filename);
            int fd = fs.open(filename);
            char buf[100];
            memset(buf, 0, sizeof buf); // 将buf清零
            fs.read(fd, buf, fs.get_size(fd));
            puts(buf);
            fs.close(fd);
        } else if (strcmp(cmd, "catat") == 0) {
            int pos;
            scanf("%s%d", filename, &pos);
            int fd = fs.open(filename);
            char buf[100];
            memset(buf, 0, sizeof buf); // 将buf清零
            fs.seek(fd, pos);
            fs.read(fd, buf, fs.get_size(fd) - pos);
            puts(buf);
            fs.close(fd);
        } else if (strcmp(cmd, "write") == 0) {
            scanf("%s", filename);
            char buf[100];
            scanf("%s", buf);
            int fd = fs.open(filename);
            fs.write(fd, buf, strlen(buf));
            fs.close(fd);
        } else if (strcmp(cmd, "write2") == 0) { // 在不关闭文件的情况下连写两次
            scanf("%s", filename);
            char buf1[100];
            scanf("%s", buf1);
            char buf2[100];
            scanf("%s", buf2);
            int fd = fs.open(filename);
            fs.write(fd, buf1, strlen(buf1));
            fs.write(fd, buf2, strlen(buf2));
            fs.close(fd);
        } else if (strcmp(cmd, "writeat") == 0) {
            scanf("%s", filename);
            int pos;
            char buf[100];
            scanf("%d%s", &pos, buf);

            int fd = fs.open(filename);
            fs.seek(fd, pos);
            fs.write(fd, buf, strlen(buf));
            fs.close(fd);
        } else if (strcmp(cmd, "help") == 0) {
            puts("quit\t\t\t\t退出");
            puts("format\t\t\t\t格式化磁盘");
            puts("touch 文件名\t\t\t创建一个新文件");
            puts("rm 文件名\t\t\t删除一个文件");
            puts("ls\t\t\t\t列出所有文件");
            puts("cat 文件名\t\t\t显示文件内容");
            puts("write 文件名 内容\t\t将内容写入文件");
            puts("write2 文件名 内容一 内容二\t将内容一、内容二相继写入文件");
            puts("writeat 文件名 位置 内容\t将内容写入文件的指定位置");
            puts("catat 文件名 位置\t\t从文件的指定位置处开始显示");
        }
    }

    return 0;
}
