#ifndef FS_H
#define FS_H

#include <stdio.h>
#include <string.h>
#include <assert.h>

// ������С(��λ���ֽ�)
const int SECTOR_SIZE = 4096;

// �����ֽ���������Ҫ��������
int bytes2sectors(int nBytes)
{
    int nSectors = nBytes / SECTOR_SIZE;
    int remainder = nBytes % SECTOR_SIZE;

    if (remainder == 0)
        return nSectors;
    else
        return nSectors + 1;
}

// �������(��ʵ�൱�ڴ��̾���)
class Disk {
	FILE* fp;
	bool ok;
public:
	Disk()
	{
		fp = fopen("disk", "r+b");
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

//-------------------------����0------------------------------
// Ŀ¼���С32�ֽ�
struct DirEntry {
	int allocated; // 0�����Ŀ¼����У�1�����Ŀ¼���ѷ����ȥ
	char name[20]; // �ļ���
	int size;  // ��λ�ֽ�
	int sector;  // �ļ���ռ��һ�������ı�š����Ϊ-1����ʾ��δ�����ļ�����ռ�
};

typedef void (*DirEntryFunc)(DirEntry* e);

// ��0�������Ľṹ
struct Sector0 {
    int formatted; // 1��ʾ�����Ѹ�ʽ����0��ʾ��δ��ʽ����
    char nouse[28]; //
    DirEntry rootDir[127]; // ��Ŀ¼������ܰ���(4096-4-28)/32=127��Ŀ¼��
};

const int TOTAL_ENTRIES_ROOT = 127; // ��Ŀ¼�ɰ�����Ŀ¼����Ŀ


//-------------------------����1------------------------------
// ���������� ֮ �����С8�ֽ�
// ��1�������ǿ�������������ܴ��256������
struct FreeSectorTableEntry {
	int first_free_sector;  // ��һ�����е��������
	int total;   // �����ٸ��������е�����
};

// ��1�������Ľṹ
struct Sector1 {
    int count; // fst�ı�����,4�ֽ�
    int nouse; // 4�ֽ�
     // ��������������ܰ���(4096-4-4)/8=511��Ŀ¼��
    FreeSectorTableEntry fst[511];
};

const int TOTAL_ENTRIES_FST = 511;  // ����������ɰ����ı�����Ŀ

//---------------------------�ļ�ϵͳ----------------------
class FileSystem {
	Disk* disk;
	Sector0 sector0;
	Sector1 sector1;
	//DirEntry rootDir[SECTOR_SIZE / sizeof(DirEntry)];
	DirEntry* rootDir;
	//FreeSectorTableEntry fst[SECTOR_SIZE / sizeof(FreeSectorTableEntry)];
	FreeSectorTableEntry* fst;

	int fst_count; // ǧ��С�ģ����ʱ�̱������������sector1.count��һ��
public:
	FileSystem(Disk* d)
	{
		disk = d;

		// �Ӵ��̶����Ŀ¼����Ŀ¼λ�ڵ�0������
		disk->read(0, &sector0);
		rootDir = sector0.rootDir;

		// �Ӵ��̶����������������������λ�ڵ�1������
		disk->read(1, &sector1);
		fst = sector1.fst;
		fst_count = sector1.count;
	}

	~FileSystem()
	{
		// ����Ŀ¼д�ش��̵ĵ�0��������
		disk->write(0, &sector0);

		// ������������д�ش��̵ĵ�1������
		sector1.count = fst_count;
		disk->write(1, &sector1);
	}

	// �жϴ����Ƿ��Ѿ���ʽ��
	bool is_formatted()
	{
        return sector0.formatted == 1;
	}

	// �Դ��̽��и�ʽ�������ڴ����Ͻ�����Ӧ�����ݽṹ
	void format()
	{
		sector0.formatted = 1;
		for (int i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
            rootDir[i].allocated = 0;
		}
		disk->write(0, &sector0);

		fst_count = 1;
		sector1.count = fst_count;
		fst[0].first_free_sector = 2;
        fst[0].total = 998; // ������̴�СΪ2+998=1000������
		disk->write(1, &sector1);
	}


	// ����Ŀ¼����������Ŀ¼����ص�����
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
//	void remove(const char* filename)
//	{
//	}

	bool newfile(const char* filename)
	{
		// �ҵ�һ�����е�Ŀ¼��
		int i;
		for (i = 0; i < TOTAL_ENTRIES_ROOT; i++) {
			if (rootDir[i].allocated == 0)
				break;
		}

		if (i == TOTAL_ENTRIES_ROOT) {
			puts("�Ѿ�û�п���Ŀ¼��");
			return false;
		}

		rootDir[i].allocated = 1;
		strcpy(rootDir[i].name, filename);
		rootDir[i].size = 0;
		rootDir[i].sector = -1; // ��ʾ��δ�����ļ�����ռ�

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

		if (rootDir[fd].sector == -1) { // ��ʾ��δ����ռ䣬���ڷ���
            // �ڿ������������ҵ��㹻��������ռ䣬�ͷ���
            int j;
            for (j = 0; j < fst_count; j++) {
                if (fst[j].total >= nSectors) {
                    break;
                }
            }

            assert(j < fst_count); // Ϊ�����������һ�����ҵ�

            rootDir[fd].sector = fst[j].first_free_sector;
            fst[j].total -= nSectors;
            fst[j].first_free_sector += nSectors;
		}
		else {
            // Ϊ�������������ǰ����Ŀռ��С�������պ���������
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

