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

// Ŀ¼���С32�ֽ�
// ��0�������Ǹ�Ŀ¼���պ��ܴ��4096/32=128��Ŀ¼�� 
struct DirEntry {
	int allocated; // 0�����Ŀ¼����У�1�����Ŀ¼���ѷ����ȥ 
	char name[20]; // �ļ��� 
	int size;  // ��λ�ֽ� 
	int sector;  // �ļ���ռ��һ�������ı�� 
};

// ���������� ֮ �����С16�ֽ�
// ��1�������ǿ�������������ܴ��256������ 
struct FreeSectorTableEntry {
	int allocated; // 0����ñ�����У�1����ñ����ѷ����ȥ 
	int first_free_sector;  // ��һ�����е�������� 
	int total;   // �����ٸ��������е����� 
	int nouse;  // ռ��λ�ã�����Ϊ����һ�������ܹ��洢������������ 
};

class FileSystem {
	Disk* disk;
	DirEntry rootDir[SECTOR_SIZE / sizeof(DirEntry)];
	FreeSectorTableEntry fst[SECTOR_SIZE / sizeof(FreeSectorTableEntry)]; 
public:
	FileSystem(Disk* disk)
	{
		this->disk = disk;
		
		// �Ӵ��̶����Ŀ¼����Ŀ¼λ�ڵ�0������ 
		disk->read(0, rootDir);
		
		// �Ӵ��̶����������������������λ�ڵ�1������ 
		disk->read(1, fst);
	}

	~FileSystem()
	{
		// ����Ŀ¼д�ش��̵ĵ�0�������� 
		disk->write(0, rootDir);
		
		// ������������д�ش��̵ĵ�1������
		disk->write(1, fst); 
	}
	
//	void remove(const char* filename)
//	{
//	}
	
	bool newfile(const char* filename)
	{
		const int total_entries_root = sizeof rootDir / sizeof rootDir[0];
		// �ҵ�һ�����е�Ŀ¼��
		int i;
		for (i = 0; i < total_entries_root; i++) {
			if (rootDir[i].allocated == 0)
				break;
		}
		
		if (i == total_entries_root) {
			puts("�Ѿ�û�п���Ŀ¼��");
			return false;
		}
	
		rootDir[i].allocated = 1;
		strcpy(rootDir[i].name, filename);
		rootDir[i].size = 0;
		
		// �ڿ������������ҵ���10��������С�������ռ䣬�ͷ���
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

