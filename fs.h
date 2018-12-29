#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <string.h>

const int SECTOR_SIZE = 4096;

class Disk {
	FILE* fp;
public:
	Disk()
	{
		fp = fopen("disk", "r+");
		if (!fp)
			puts("dfdasf");
	}
	
	~Disk()
	{
		fclose(fp);
	}
	
	void read(int i, void* sector)
	{
		fseek(fp, i * SECTOR_SIZE, SEEK_SET);
		fread(sector, 1, SECTOR_SIZE, fp);

	}
	
	void write(int i, const void* sector)
	{
		fseek(fp, i * SECTOR_SIZE, SEEK_SET);
		fwrite(sector, 1, SECTOR_SIZE, fp);
	}
};

// 目录项，大小32字节
// 第0个扇区是根目录，刚好能存放4096/32=128个目录项 
struct DirEntry {
	int allocated; // 0代表该目录项空闲；1代表该目录项已分配出去 
	char name[20]; // 文件名 
	int size;  // 单位字节 
	int sector;  // 文件所占第一个扇区的编号 
};

// 空闲扇区表 之 表项，大小16字节
// 第1个扇区是空闲扇区表，最多能存放256个表项 
struct FreeSectorTableEntry {
	int allocated; // 0代表该表项空闲；1代表该表项已分配出去 
	int first_free_sector;  // 第一个空闲的扇区编号 
	int total;   // 共多少个连续空闲的扇区 
	int nouse;  // 占个位置，仅仅为了让一个扇区能够存储下整数个表项 
};

class FileSystem {
	Disk* disk;
	DirEntry rootDir[SECTOR_SIZE / sizeof(DirEntry)];
	FreeSectorTableEntry fst[SECTOR_SIZE / sizeof(FreeSectorTableEntry)]; 
public:
	FileSystem(Disk* disk)
	{
		this->disk = disk;
		
		// 从磁盘读入根目录。根目录位于第0个扇区 
		disk->read(0, rootDir);
		
		// 从磁盘读入空闲扇区表。空闲扇区表位于第1个扇区 
		disk->read(1, fst);
	}

	~FileSystem()
	{
		// 将根目录写回磁盘的第0个扇区。 
		disk->write(0, rootDir);
		
		// 将空闲扇区表写回磁盘的第1个扇区
		disk->write(1, fst); 
	}
	
//	void remove(const char* filename)
//	{
//	}
	
	bool newfile(const char* filename)
	{
		const int total_entries_root = sizeof rootDir / sizeof rootDir[0];
		// 找到一个空闲的目录项
		int i;
		for (i = 0; i < total_entries_root; i++) {
			if (rootDir[i].allocated == 0)
				break;
		}
		
		if (i == total_entries_root) {
			puts("已经没有空闲目录项");
			return false;
		}
	
		rootDir[i].allocated = 1;
		strcpy(rootDir[i].name, filename);
		rootDir[i].size = 0;
		
		// 在空闲扇区表里找到有10个扇区大小的连续空间，就分配
		const int total_entries_fst = sizeof fst / sizeof fst[0];
		int j;
		for (j = 0; j < total_entries_fst; j++) {
			if (fst[j].allocated && fst[j].total >= 5) {
				break;
			}
		}
		 
		 
		rootDir[i].sector = fst[j].first_free_sector;
		fst[j].total -= 2;
		fst[j].first_free_sector += 2;
		
		return true;
	}
	
	
	int open(const char* filename)
	{
		const int total_entries_root = sizeof rootDir / sizeof rootDir[0];
		int i;
		for (i = 0; i < total_entries_root; i++) {
			if (rootDir[i].allocated == 1 && strcmp(rootDir[i].name, filename) == 0)
				break;
		}
		
		if (i == total_entries_root) {
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

