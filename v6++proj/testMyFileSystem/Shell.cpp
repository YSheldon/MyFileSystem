#include "Shell.h"
#include "TestMyFileSystem.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;

int Shell::char2int(char *number)
{
	int length = strlen(number);
	int n = 0;
	int power = 1;
	for (int i = length - 1; i >= 0; i--)
	{
		n += (number[i] - '0') * power;
		power *= 10;
	}
	return n;
}

void Shell::AnalyzeCommend(char* lineInput, int &argc, char argv[4][10])
{
	string temp;
	argc = 0;
	int length = strlen(lineInput);
	for (int i = 0; i < length; i++)
	{
		if (lineInput[i] == ' ')
		{
			strcpy(argv[argc], temp.c_str());
			argc++;
			for (int j = strlen(argv[argc]); j < 10; j++)
				argv[argc][j] = 0;
			temp = "";
		}
		else
		{
			temp += lineInput[i];
		}
	}
	strcpy(argv[argc], temp.c_str());
	argc++;
	for (int j = strlen(argv[argc]); j < 10; j++)
		argv[argc][j] = 0;
}

int Shell::FunctionNumber(char* name)
{
	vector<string> functions = { "ls", "cd", "fopen", "fclose", "fread", "fwrite",
		"flseek", "fcreat", "fdelete" };
	int i;
	for (i = 0; i < functions.size(); i++)
	{
		if (strcmp(name,functions[i].c_str()) == 0)
			return i;
	}
	return -1;
}

void Shell::CallFunction(int argc, char argv[4][10])
{
	int number = FunctionNumber(argv[0]);
	enum class functions { ls, cd, fopen, fclose, fread, fwrite, flseek, fcreat, fdelete };
	if (number == -1)
	{
		cout << "error function name" << endl;
		return;
	}
	functions fnumber = (functions)number;
	switch (fnumber)
	{
		case functions::ls:
		{
			ls();
			break;
		}
		case functions::cd:
		{
			cd(argv[1]);
			break;
		}
		case functions::fopen:
		{
			fopen(argv[1], char2int(argv[2]));
			break;
		}
		case functions::fclose:
		{
			fclose(char2int(argv[1]));
			break;
		}
		case functions::fwrite:
		{
			char buffer[512];
			cout << "please write something : " << endl;
			gets(buffer);
			fwrite(char2int(argv[1]), buffer, char2int(argv[2]));
			break;
		}
		case functions::fread:
		{
			char buffer[512] = { 0 };
			fread(char2int(argv[1]), buffer, char2int(argv[2]));
			break;
		}
		case functions::flseek:
		{
			flseek(char2int(argv[1]), char2int(argv[2]));
			break;
		}
		case functions::fcreat:
		{
			fcreat(argv[1], char2int(argv[2]));
			break;
		}
		case functions::fdelete:
		{
			fdelete(argv[1]);
			break;
		}
	}
	
}

void Shell::shell()
{
	char lineInput[50];
	int argc;
	char argv[4][10];
	int root = 0; //ÅÐ¶ÏÊÇ·ñ³õ´ÎÆô¶¯
	while (1)
	{
		if (strcmp(lineInput, "exit") == 0)
			break;
		if (root)
		{
			cout << CURRENT_PATH << " $:";
			AnalyzeCommend(gets(lineInput), argc, argv);
			CallFunction(argc, argv);
		}
		else
		{
			CURRENT_DIR = *m_FileManager.rootDirInode;
			root = 1;
		}
	}
}


