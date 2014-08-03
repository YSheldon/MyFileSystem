#include "MyBuf.h"
#include "TestMyFileSystem.h"
#include <string>
#include <iostream>
using namespace std;
const std::string DISK_PATH = "C:\\Users\\saika\\Documents\\GitHub\\MyFileSystem\\buildproj\\MyDiskInit\\bin\\Debug\\MyDisk.img"; // 磁盘文件路径


Buf::Buf()
{
	blkno = 0;
	memset(buffer, 0, 512);
	nextBuf = NULL;
}

BufferManager::BufferManager()
{
	Buf *temp = &bFreeList;
	for (int i = 0; i < NBUF; i++)
	{
		temp->nextBuf = new Buf();
		temp = temp->nextBuf;
	}
}

BufferManager::~BufferManager()
{
	Buf *temp = bFreeList.nextBuf;
	Buf *t;
	while (temp)
	{
		t = temp;
		temp = temp->nextBuf;
		delete(t);
	}
}

Buf* BufferManager::GetBlk(int blkno)
{
	Buf* temp = &bFreeList;
	while (temp->nextBuf != NULL)
	{
		if (temp->nextBuf->blkno == blkno)
			break;
		temp = temp->nextBuf;
	}
	if (temp->nextBuf == NULL)
		temp = &bFreeList;
	Buf* reBuf = temp->nextBuf;
	temp->nextBuf = reBuf->nextBuf;
	return reBuf;
}

void BufferManager::Brelse(Buf* bp)
{
	bp->nextBuf = NULL;
	Buf* temp = &bFreeList;
	while (temp->nextBuf != NULL)
		temp = temp->nextBuf;
	temp->nextBuf = bp;
}


Buf* BufferManager::Bread(int blkno)
{
	Buf* reBuf = GetBlk(blkno);
	if (reBuf->blkno == blkno)
		return reBuf;
	reBuf->blkno = blkno;
	int fd = _open(DISK_PATH.c_str(), O_RDWR);
	_lseek(fd, blkno * 512, SEEK_SET);
	_read(fd, &reBuf->buffer, 512);
	_close(fd);
	return reBuf;
}

void BufferManager::Bwrite(Buf* bp)
{
	int fd = _open(DISK_PATH.c_str(), O_RDWR);
	_lseek(fd, bp->blkno * 512, SEEK_SET);
	_write(fd, bp->buffer, 512);
	_close(fd);
}

void BufferManager::ClrBuf(Buf *bp)
{
	/* 将缓冲区中数据清零 */
	for (unsigned int i = 0; i < BufferManager::BUFFER_SIZE; i++)
	{
		bp->buffer[i] = 0;
	}
	return;
}