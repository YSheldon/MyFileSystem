#ifndef TESTMYFILESYSTEM_H
#define TESTMYFILESYSTEM_H
#include "OpenFileManager.h"
#include "File.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "MyBuf.h"
#include <string>

extern InodeTable m_InodeTable;
extern BufferManager m_BufferManager;
extern FileSystem m_FileSystem;
extern SuperBlock m_SuperBlock;
extern OpenFileTable m_OpenFileTable;
extern Inode CURRENT_DIR;
extern std::string CURRENT_PATH; // µ±Ç°Â·¾¶
extern int ERRNO;
#endif