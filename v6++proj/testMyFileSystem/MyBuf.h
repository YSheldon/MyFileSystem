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
	static const int NBUF = 15;			/* ������ƿ顢������������ */
	static const int BUFFER_SIZE = 512;	/* ��������С�� ���ֽ�Ϊ��λ */

public:
	BufferManager();
	~BufferManager();

	Buf* GetBlk(int blkno);				/* ����һ�黺�棬���ڶ�д�豸dev�ϵ��ַ���blkno��*/
	void Brelse(Buf* bp);				/* �ͷŻ�����ƿ�buf */
	Buf* Bread(int blkno);				/* ��һ�����̿顣blknoΪĿ����̿��߼���š� */
	void Bwrite(Buf* bp);				/* дһ�����̿� */
	void ClrBuf(Buf *bp);				

//private:
	Buf bFreeList;						/* ���ɻ�����п��ƿ� */
};


#endif


