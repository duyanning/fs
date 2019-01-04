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
        puts("������δ��ʽ��������ִ��format�����ʽ�����̡�");
    }

    puts("ִ��help������г��������ִ��quit�����˳���");
    char cmd[20];
    char filename[50];

    for (;;) {
        printf("> ");
        scanf("%s", cmd);
        if (!fs.is_formatted() && strcmp(cmd, "format") != 0 && strcmp(cmd, "quit") != 0) {
            puts("������δ��ʽ��������ִ��format�����ʽ�����̡�");
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
            memset(buf, 0, sizeof buf); // ��buf����
            fs.read(fd, buf, fs.get_size(fd));
            puts(buf);
            fs.close(fd);
        } else if (strcmp(cmd, "catat") == 0) {
            int pos;
            scanf("%s%d", filename, &pos);
            int fd = fs.open(filename);
            char buf[100];
            memset(buf, 0, sizeof buf); // ��buf����
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
        } else if (strcmp(cmd, "write2") == 0) { // �ڲ��ر��ļ����������д����
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
            puts("quit\t\t\t\t�˳�");
            puts("format\t\t\t\t��ʽ������");
            puts("touch �ļ���\t\t\t����һ�����ļ�");
            puts("rm �ļ���\t\t\tɾ��һ���ļ�");
            puts("ls\t\t\t\t�г������ļ�");
            puts("cat �ļ���\t\t\t��ʾ�ļ�����");
            puts("write �ļ��� ����\t\t������д���ļ�");
            puts("write2 �ļ��� ����һ ���ݶ�\t������һ�����ݶ����д���ļ�");
            puts("writeat �ļ��� λ�� ����\t������д���ļ���ָ��λ��");
            puts("catat �ļ��� λ��\t\t���ļ���ָ��λ�ô���ʼ��ʾ");
        }
    }

    return 0;
}
