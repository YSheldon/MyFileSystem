#ifndef SHELL_H
#define	SHELL_H
#include <string>
class Shell
{
public:
	void AnalyzeCommend(char* lineInput, int &argc, char argv[4][10]);
	int FunctionNumber(char* name);
	void CallFunction(int argc, char argv[4][10]);
	void shell();
	int char2int(char *number);
};
#endif // !SHELL.H

