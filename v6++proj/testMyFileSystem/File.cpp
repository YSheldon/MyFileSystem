#include "File.h"

File::File()
{
	this->f_count = 0;
	this->f_flag = 0;
	this->f_offset = 0;
	this->f_inode = NULL;
}
File::~File()
{
	//nothing to do here
}

IOParameter::IOParameter(unsigned char* base, int offset, int count) :m_Base(base), m_Offset(offset), m_Count(count)
{

}

IOParameter::IOParameter()
{
	this->m_Base = 0;
	this->m_Count = 0;
	this->m_Offset = 0;
}

IOParameter::~IOParameter()
{
	//nothing to do here
}
