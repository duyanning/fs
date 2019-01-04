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
//---------------------------打开文件表----------------------
// 打开文件表的表项
struct OpenedFileTableEntry {
    bool allocated;
    DirEntry* dir_entry; // 文件的目录项
    int pos; // 当前读写位置
};

const int TOTAL_ENTRIES_OFT = 100;  // 打开文件表可包含的表项数目

// 打开文件表
OpenedFileTableEntry opened_file_table[TOTAL_ENTRIES_OFT];
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

		// 初始化打开文件表(全部清零)
		memset(opened_file_table, 0, sizeof opened_file_table);
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
	    DirEntry* de = opened_file_table[fd].dir_entry;
	    return de->size;

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


	// 返回打开文件表中的位置，如果文件不存在，就返回-1
	// 尚未考虑文件之前已经打开的情形
	int open(const char* filename)
	{
	    // 先在根目录中看看该文件是否存在
		int iDir;
		for (iDir = 0; iDir < TOTAL_ENTRIES_ROOT; iDir++) {
			if (rootDir[iDir].allocated == 1 && strcmp(rootDir[iDir].name, filename) == 0)
				break;
		}

		if (iDir == TOTAL_ENTRIES_ROOT) {
			return -1;
		}

		// 在打开文件表中分配一个表项
		int iOft;
		for (iOft = 0; iOft < TOTAL_ENTRIES_OFT; iOft++) {
			if (opened_file_table[iOft].allocated == 0)
				break;
		}

		if (iOft == TOTAL_ENTRIES_OFT) {
			puts("已达到打开文件的最大数目");
			return -1;
		}

		opened_file_table[iOft].allocated = 1;
		opened_file_table[iOft].dir_entry = &rootDir[iDir];
		opened_file_table[iOft].pos = 0;


		return iOft;

	}
//
	void close(int fd)
	{
	    opened_file_table[fd].allocated = 0;
	}
//
	void write(int fd, const void* buffer, int size)
	{
	    DirEntry* de = opened_file_table[fd].dir_entry;

	    int nSectors = bytes2sectors(size);

		if (de->sector == -1) { // 表示还未分配空间，现在分配
            // 在空闲扇区表里找到足够大的连续空间，就分配
            int j;
            for (j = 0; j < sector1.fst_count; j++) {
                if (fst[j].total >= nSectors) {
                    break;
                }
            }

            assert(j < sector1.fst_count); // 为简单起见，假设一定能找到

            de->sector = fst[j].first_free_sector;
            fst[j].total -= nSectors;
            fst[j].first_free_sector += nSectors;

            de->size = size;
		}
		else {
            // 为简单起见，假设先前分配的空间大小能满足日后所有需求
            assert(bytes2sectors(de->size) >= nSectors);
		}



		int first_sector = de->sector;

		char* p = (char*)buffer;


		// 需要考虑这样的情形：起始与结束位置，都不在扇区边界处。
		// 可能起始位置位于起始扇区的中间某处
		// 结束位置也位于结束扇区的中间某处
		// 对于这两个扇区要拼装出来在写入磁盘
		// 对于起始扇区，拼装出来的内容来自磁盘上起始扇区的现有内容与buffer
		// 对于结束扇区，拼装出来的内容来自磁盘上结束扇区的现有内容与buffer
		// 注意：所有begin_、end_区间皆为闭区间区间
		int begin_pos = opened_file_table[fd].pos;
		int end_pos = begin_pos + size - 1;
        int begin_sector = first_sector + begin_pos / SECTOR_SIZE;
        int end_sector = first_sector + end_pos / SECTOR_SIZE;
        int begin_pos_in_sector = begin_pos % SECTOR_SIZE;
        int end_pos_in_sector = end_pos % SECTOR_SIZE;

        for (int iSector = begin_sector; iSector <= end_sector; iSector++) {
            if (iSector == begin_sector && iSector == end_sector) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(sec + begin_pos_in_sector, p, size);
                disk->write(iSector, sec);
                p += size;
            }
            else if (iSector == begin_sector) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(sec + begin_pos_in_sector, p, SECTOR_SIZE - begin_pos_in_sector);
                disk->write(iSector, sec);
                p += SECTOR_SIZE - begin_pos_in_sector;
            }
            else if (iSector == end_sector - 1) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(sec, p, end_pos_in_sector + 1);
                disk->write(iSector, sec);
                p += end_pos_in_sector;
            }
            else {
                disk->write(iSector, p);
                p += SECTOR_SIZE;
            }
        }

        opened_file_table[fd].pos += size; // 移动读写位置
        if (opened_file_table[fd].pos > de->size) {
            de->size = opened_file_table[fd].pos;
        }

	}
//
	void read(int fd, const void* buffer, int size)
	{
	    DirEntry* de = opened_file_table[fd].dir_entry;

	    // 为简单起见，不要让读写位置越过文件末尾
	    assert(opened_file_table[fd].pos + size <= de->size);

		int first_sector = de->sector;
		char* p = (char*)buffer;

		int begin_pos = opened_file_table[fd].pos;
		int end_pos = begin_pos + size - 1;
        int begin_sector = first_sector + begin_pos / SECTOR_SIZE;
        int end_sector = first_sector + end_pos / SECTOR_SIZE;
        int begin_pos_in_sector = begin_pos % SECTOR_SIZE;
        int end_pos_in_sector = end_pos % SECTOR_SIZE;

        for (int iSector = begin_sector; iSector <= end_sector; iSector++) {
            if (iSector == begin_sector && iSector == end_sector) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(p, sec + begin_pos_in_sector, size);
                p += size;
            }
            else if (iSector == begin_sector) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(p, sec + begin_pos_in_sector, SECTOR_SIZE - begin_pos_in_sector);
                p += SECTOR_SIZE - begin_pos_in_sector;
            }
            else if (iSector == end_sector - 1) {
                char sec[SECTOR_SIZE];
                disk->read(iSector, sec);
                memcpy(p, sec, end_pos_in_sector + 1);
                p += end_pos_in_sector;
            }
            else {
                disk->read(iSector, p);
                p += SECTOR_SIZE;
            }
        }

        opened_file_table[fd].pos += size; // 移动读写位置
        if (opened_file_table[fd].pos > de->size) {
            de->size = opened_file_table[fd].pos;
        }


//		int n = size / SECTOR_SIZE;
//		int remainder = size % SECTOR_SIZE;
//
//		int i;
//		for (i = first_sector; i < first_sector + n; i++) {
//			disk->read(i, p);
//			p += SECTOR_SIZE;
//		}
//
//		if (remainder != 0) {
//			char buffer[SECTOR_SIZE];
//			disk->read(i, buffer);
//			memcpy(p, buffer, remainder);
//		}
	}
};

#endif

