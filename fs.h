#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

// 扇区大小(单位：字节)
const int SECTOR_SIZE = 4096;

// 根据字节数计算需要的扇区数
int bytes2sectors(int nBytes)
{
    int nSectors = nBytes / SECTOR_SIZE;
    int remainder = nBytes % SECTOR_SIZE;

    if (remainder == 0)
        return nSectors;
    else
        return nSectors + 1;
}

// 代表磁盘(其实相当于磁盘镜像)
class Disk {
	FILE* fp;
	bool ok;
public:
	Disk()
	{
	    // 打开已经存在的disk文件
		fp = fopen("disk", "r+b");

		// disk文件不存在就创建一个
		if (!fp)
            fp = fopen("disk", "w+b");

		if (fp) {
			ok = true;
		}
		else {
            ok = false;
		}
	}

	~Disk()
	{
		fclose(fp);
	}

	bool is_ok()
	{
        return ok;
	}

	// 读扇区i。请确保sector所指缓冲区有SECTOR_SIZE字节大
	void read(int i, void* sector)
	{
		fseek(fp, i * SECTOR_SIZE, SEEK_SET);
		fread(sector, 1, SECTOR_SIZE, fp);

	}

	// 写扇区i。请确保sector所指缓冲区有SECTOR_SIZE字节大
	void write(int i, const void* sector)
	{
		fseek(fp, i * SECTOR_SIZE, SEEK_SET);
		fwrite(sector, 1, SECTOR_SIZE, fp);
	}
};

//-------------------------扇区0------------------------------
// 目录项，大小32字节
struct DirEntry {
	int allocated; // 0代表该目录项空闲；1代表该目录项已分配出去
	char name[20]; // 文件名
	int size;  // 单位字节
	int sector;  // 文件所占第一个扇区的编号。如果为-1，表示还未给该文件分配空间
};

typedef void (*DirEntryFunc)(DirEntry* e);

// 第0个扇区的结构
struct Sector0 {
    int formatted; // 1表示磁盘已格式化，0表示尚未格式化。
    char nouse[28]; //
    DirEntry rootDir[127]; // 根目录，最多能包含(4096-4-28)/32=127个目录项
};

const int TOTAL_ENTRIES_ROOT = 127; // 根目录可包含的目录项数目

// 确保你的修改不会导致数据结构的大小不等于一扇区
static_assert(sizeof(Sector0) == SECTOR_SIZE, "你的修改导致数据结构的大小不等于一扇区");
//-------------------------扇区1------------------------------
// 空闲扇区表 之 表项，大小8字节
// 第1个扇区是空闲扇区表，最多能存放256个表项
struct FreeSectorTableEntry {
	int first_free_sector;  // 第一个空闲的扇区编号
	int total;   // 共多少个连续空闲的扇区
};

// 第1个扇区的结构
struct Sector1 {
    int fst_count; // fst的表项数,4字节
    int nouse; // 4字节
     // 空闲扇区表，最多能包含(4096-4-4)/8=511个目录项
    FreeSectorTableEntry fst[511];
};

const int TOTAL_ENTRIES_FST = 511;  // 空闲扇区表可包含的表项数目

// 确保你的修改不会导致数据结构的大小不等于一扇区
static_assert(sizeof(Sector1) == SECTOR_SIZE, "你的修改导致数据结构的大小不等于一扇区");
//---------------------------文件系统----------------------
class FileSystem {
	Disk* disk;
	Sector0 sector0;
	Sector1 sector1;
	//DirEntry rootDir[SECTOR_SIZE / sizeof(DirEntry)];
	DirEntry* rootDir;
	//FreeSectorTableEntry fst[SECTOR_SIZE / sizeof(FreeSectorTableEntry)];
	FreeSectorTableEntry* fst;

public:
	FileSystem(Disk* d)
	{
		disk = d;

		// 从磁盘读入根目录。根目录位于第0个扇区
		disk->read(0, &sector0);
		rootDir = sector0.rootDir;

		// 从磁盘读入空闲扇区表。空闲扇区表位于第1个扇区
		disk->read(1, &sector1);
		fst = sector1.fst;
	}

	~FileSystem()
	{
		// 将根目录写回磁盘的第0个扇区。
		disk->write(0, &sector0);

		// 将空闲扇区表写回磁盘的第1个扇区
		disk->write(1, &sector1);
	}

	// 判断磁盘是否已经格式化
	bool is_formatted()
	{
        return sector0.formatted == 1;
	}

	// 对磁盘进行格式化，即在磁盘上建立相应的数据结构
	void format()
	{
		sector0.formatted = 1;
		for (int i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
            rootDir[i].allocated = 0;
		}
		disk->write(0, &sector0);

		sector1.fst_count = 1;
		fst[0].first_free_sector = 2; // 前两个扇区已做它用
        fst[0].total = 998; // 假设磁盘大小为2+998=1000个扇区
		disk->write(1, &sector1);
	}


	// 遍历目录，并将各个目录项传给回调函数
	void traverse(DirEntryFunc callback)
	{
 		for (int i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
			if (rootDir[i].allocated == 1) {
                callback(&rootDir[i]);
			}
		}
	}

	int get_size(int fd)
	{
	    return rootDir[fd].size;

	}

	void remove(const char* filename)
	{
	    puts("not implemented!");
	}

	bool newfile(const char* filename)
	{
		// 找到一个空闲的目录项
		int i;
		for (i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
			if (rootDir[i].allocated == 0)
				break;
		}

		if (i == TOTAL_ENTRIES_ROOT) {
			puts("已经没有空闲目录项");
			return false;
		}

		rootDir[i].allocated = 1;
		strcpy(rootDir[i].name, filename);
		rootDir[i].size = 0;
		rootDir[i].sector = -1; // 表示还未给该文件分配空间

		return true;
	}


	int open(const char* filename)
	{
		int i;
		for (i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
			if (rootDir[i].allocated == 1 && strcmp(rootDir[i].name, filename) == 0)
				break;
		}

		if (i == TOTAL_ENTRIES_ROOT) {
			return -1;
		}

		return i;

	}
//
	void close(int fd)
	{
	}
//
	void write(int fd, const void* buffer, int size)
	{
	    int nSectors = bytes2sectors(size);

		if (rootDir[fd].sector == -1) { // 表示还未分配空间，现在分配
            // 在空闲扇区表里找到足够大的连续空间，就分配
            int j;
            for (j = 0; j < sector1.fst_count; j++) {
                if (fst[j].total >= nSectors) {
                    break;
                }
            }

            assert(j < sector1.fst_count); // 为简单起见，假设一定能找到

            rootDir[fd].sector = fst[j].first_free_sector;
            fst[j].total -= nSectors;
            fst[j].first_free_sector += nSectors;
		}
		else {
            // 为简单起见，假设先前分配的空间大小能满足日后所有需求
            assert(bytes2sectors(rootDir[fd].size) >= nSectors);
		}

		rootDir[fd].size = size;

		int first_sector = rootDir[fd].sector;

		char* p = (char*)buffer;


		int n = size / SECTOR_SIZE;
		int remainder = size % SECTOR_SIZE;

		int i;
		for (i = first_sector; i < first_sector + n; i++) {
			disk->write(i, p);
			p += SECTOR_SIZE;
		}

		if (remainder != 0) {
			char buf[SECTOR_SIZE];
			memcpy(buf, p, remainder);
			disk->write(i, buf);
		}



	}
//
	void read(int fd, const void* buf, int size)
	{
		int first_sector = rootDir[fd].sector;
		char* p = (char*)buf;


		int n = size / SECTOR_SIZE;
		int remainder = size % SECTOR_SIZE;

		int i;
		for (i = first_sector; i < first_sector + n; i++) {
			disk->read(i, p);
			p += SECTOR_SIZE;
		}

		if (remainder != 0) {
			char buf[SECTOR_SIZE];
			disk->read(i, buf);
			memcpy(p, buf, remainder);
		}
	}
};

#endif

