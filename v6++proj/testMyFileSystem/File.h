#ifndef FILE_H
#define FILE_H

#include "INode.h"

/*
* ���ļ����ƿ�File�ࡣ
* �ýṹ��¼�˽��̴��ļ�
* �Ķ���д�������ͣ��ļ���дλ�õȵȡ�
*/
class File
{
public:
	/* Enumerate */
	enum FileFlags
	{
		FREAD = 0x1,			/* ���������� */
		FWRITE = 0x2,			/* д�������� */
	};

	/* Functions */
public:
	/* Constructors */
	File();
	/* Destructors */
	~File();


	/* Member */
	unsigned int f_flag;		/* �Դ��ļ��Ķ���д����Ҫ�� */
	Inode*	f_inode;			/* ָ����ļ����ڴ�Inodeָ�� */
	int		f_offset;			/* �ļ���дλ��ָ�� */
	int		f_count;
};

/*
* �ļ�I/O�Ĳ�����
* ���ļ�����дʱ���õ��Ķ���дƫ������
* �ֽ����Լ�Ŀ�������׵�ַ������
*/
class IOParameter
{
	/* Functions */
public:
	/* Constructors */
	IOParameter();
	IOParameter(unsigned char*, int, int);
	/* Destructors */
	~IOParameter();

	/* Members */
public:
	unsigned char* m_Base;	/* ��ǰ����д�û�Ŀ��������׵�ַ */
	int m_Offset;	/* ��ǰ����д�ļ����ֽ�ƫ���� */
	int m_Count;	/* ��ǰ��ʣ��Ķ���д�ֽ����� */
};

#endif
