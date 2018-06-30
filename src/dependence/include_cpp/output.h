#ifndef OUTPUT_H
#define OUTPUT_H

#include <string>
#include <iostream>
#include <Windows.h>
#include "libxl.h"
#include <vector>
using namespace libxl;
using namespace std;

class excleIO
{
public:
	enum Kind
	{
		XLS = 0,
		XLSX
	};
	excleIO(excleIO::Kind kind);
	void generateTestFile(const std::string fileDir, const std::string& fileName);


private:

	Book * book;

};


class output
{
public:
	output(const std::string& fileName);

private:
	Book * book;
};




void writeResultToXlsx(const std::string & fileName, std::vector<std::string>& files);
void readFilePathFromXlsx(const std::string & fileName, std::vector<std::string>& files);
void generateXlsxFlie(const std::string & xlsxFileName, std::vector<std::string>& filenames );
void writeOneIndexToExcle(const std::string & fileName,int index, std::string plate, std::string color);

wchar_t * Utf_8ToUnicode(const char* szU8);
char* UnicodeToAnsi(const wchar_t* szStr);


#endif // !OUTPUT_H


