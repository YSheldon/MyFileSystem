#ifndef TESTMYFILESYSTEM_H
#define TESTMYFILESYSTEM_H
#include "OpenFileManager.h"
#include "File.h"
#include "FileManager.h"
#include "FileSystem.h"
#include "MyBuf.h"
#include <string>

extern FileManager m_FileManager;
extern InodeTable m_InodeTable;
extern BufferManager m_BufferManager;
extern FileSystem m_FileSystem;
extern SuperBlock m_SuperBlock;
extern OpenFileTable m_OpenFileTable;
extern Inode CURRENT_DIR;
extern std::string CURRENT_PATH; // µ±Ç°Â·¾¶
extern int ERRNO;

extern void ls();
extern void cd(char *name);
extern int fopen(char *name, int mode);
extern void fclose(int fd);
extern int fread(int fd, char *buffer, int length);
extern int fwrite(int fd, char *buffer, int length);
extern void flseek(int fd, int position);
extern int fcreat(char *name, int mode);
extern int fdelete(char *name);
#endif