#ifndef MYBUF_H
#define MYBUF_H

#include <fcntl.h>
#include <io.h>
#include <stdio.h>

class Buf
{
public:
	int blkno;
	unsigned char buffer[512];
	Buf* nextBuf;
	Buf();
};

class BufferManager
{
public:
	/* static const member */
	static const int NBUF = 15;			/* 缓存控制块、缓冲区的数量 */
	static const int BUFFER_SIZE = 512;	/* 缓冲区大小。 以字节为单位 */

public:
	BufferManager();
	~BufferManager();

	Buf* GetBlk(int blkno);				/* 申请一块缓存，用于读写设备dev上的字符块blkno。*/
	void Brelse(Buf* bp);				/* 释放缓存控制块buf */
	Buf* Bread(int blkno);				/* 读一个磁盘块。blkno为目标磁盘块逻辑块号。 */
	void Bwrite(Buf* bp);				/* 写一个磁盘块 */
	void ClrBuf(Buf *bp);				

//private:
	Buf bFreeList;						/* 自由缓存队列控制块 */
};


#endif


