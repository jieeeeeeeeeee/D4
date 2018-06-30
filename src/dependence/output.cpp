#include "output.h"

excleIO::excleIO(excleIO::Kind kind)
{
	if (kind == 0)
	{
		book = xlCreateBook();
		book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");
	}
	else if (kind == 1)
	{
		book = xlCreateXMLBook();
		book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");
	}
	else
	{
		std::cerr << "Wrong Kind !"<<std::endl;
	}
}

void excleIO::generateTestFile(const std::string fileDir, const std::string & fileName)
{

}

output::output(const std::string & fileName)
{
	book = xlCreateXMLBook();
	book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");

}

wchar_t * Utf_8ToUnicode(const char* szU8)
{
	//UTF8 to Unicode  
	//由于中文直接复制过来会成乱码，编译器有时会报错，故采用16进制形式  

	//预转换，得到所需空间的大小  
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);
	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间  
	wchar_t* wszString = new wchar_t[wcsLen + 1];
	//转换  
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);
	//最后加上'\0'  
	wszString[wcsLen] = '\0';
	return wszString;
}

char* UnicodeToAnsi(const wchar_t* szStr)
{
	int nLen = WideCharToMultiByte(CP_ACP, 0, szStr, -1, NULL, 0, NULL, NULL);
	if (nLen == 0)
	{
		return NULL;
	}
	char* pResult = new char[nLen];

	WideCharToMultiByte(CP_ACP, 0, szStr, -1, pResult, nLen, NULL, NULL);

	return pResult;

}






// wchar_t to string
void Wchar_tToString(std::string& szDst,const wchar_t *wchar)
{
	const wchar_t * wText = wchar;
	DWORD dwNum = WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, NULL, 0, NULL, FALSE);// WideCharToMultiByte的运用
	char *psText;  // psText为char*的临时数组，作为赋值给std::string的中间变量
	psText = new char[dwNum];
	WideCharToMultiByte(CP_OEMCP, NULL, wText, -1, psText, dwNum, NULL, FALSE);// WideCharToMultiByte的再次运用
	szDst = psText;// std::string赋值
	delete[]psText;// psText的清除
}

// string to wstring
void StringToWstring(std::wstring& szDst, std::string str)
{
	std::string temp = str;
	int len = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, NULL, 0);
	wchar_t * wszUtf8 = new wchar_t[len + 1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)temp.c_str(), -1, (LPWSTR)wszUtf8, len);
	szDst = wszUtf8;
	std::wstring r = wszUtf8;
	delete[] wszUtf8;
}

void readFilePathFromXlsx(const std::string & fileName, std::vector<std::string>& files)
{
	std::string path1 = fileName;
	//	std::string path1 = "test.xlsx";
	wstring  path2;
	StringToWstring(path2, path1);
	const wchar_t * path = path2.c_str();
	Book * book = xlCreateXMLBook();
	book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");

	if (book)//是否创建实例成功
	{
		if (book->load(path))
		{
			Sheet * sheet = book->getSheet(0);
			if (sheet)
			{
				int rows = sheet->lastRow();
				int cols = sheet->lastCol();
				std::cout << rows << "  " << cols << std::endl;
				for (int i = 1; i < rows; i++) {
					const wchar_t* wstr = sheet->readStr(i, 2);
					std::string str;
					Wchar_tToString(str, wstr);
					auto iter = str.begin();
					while (iter != str.end())
					{
						if (*iter == '\\')
						{
							*iter = '/';
						}
						iter++;
					}
					files.push_back(str);
				}
			}
			if (book->save(path))
			{
				//std::cout << "File example.xls has been modified." << std::endl;
				//::ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOW);
			}
			else
			{
				std::cout << book->errorMessage() << std::endl;
			}
		}
		else
		{
			std::cout << "xlsx file path is wrong !" << std::endl;
		}

		book->release();
	}
}
void generateXlsxFlie(const std::string & fileName, std::vector<std::string>& files)
{
	std::string path1 = fileName;
	//	std::string path1 = "test.xlsx";
	wstring  path2;
	StringToWstring(path2, path1);
	const wchar_t * path = path2.c_str();
	Book * book = xlCreateXMLBook();
	book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");
	if (book)//是否创建实例成功
	{
		if (book->load(path))
		{
			Sheet * sheet = book->getSheet(0);
			sheet->clear();
			if (sheet)
			{
				int rows = sheet->lastRow();
				int cols = sheet->lastCol();
				std::cout << rows << "  " << cols << std::endl;
				for (int i = 0; i < files.size(); i++) {

					std::string str = files[i];
					auto iter = str.begin();
					while (iter != str.end())
					{
						if (*iter == '\\')
						{
							*iter = '/';
						}
						iter++;
					}
					wstring wstr1;
					StringToWstring(wstr1, str);
					sheet->writeStr(i + 1, 2, wstr1.c_str());
				}
			}
			if (book->save(path))
			{
				std::cout << "File example.xls has been modified." << std::endl;
				std::string path1;
				Wchar_tToString(path1, path);
				//::ShellExecute(NULL, "open", path1.c_str(), NULL, NULL, SW_SHOW);
			}
			else
			{
				std::cout << book->errorMessage() << std::endl;
			}
		}
		else
		{
			std::cout << "*.xlsx file path is wrong !" << std::endl;
		}

		book->release();
	}
}
void writeResultToXlsx(const std::string & fileName, std::vector<std::string>& files)
{
}


void writeOneIndexToExcle(const std::string & fileName,int index, std::string plate, std::string color)
{
	std::string path1 = fileName;
	//	std::string path1 = "test.xlsx";
	wstring  path2;
	StringToWstring(path2, path1);
	const wchar_t * path = path2.c_str();
	Book * book = xlCreateXMLBook();
	book->setKey(L"Halil Kural", L"windows-2723210a07c4e90162b26966a8jcdboe");
	if (book)//是否创建实例成功
	{
		if (book->load(path))
		{
			Sheet * sheet = book->getSheet(0);
			if (sheet)
			{
				wstring wstr1, wstr2;
				StringToWstring(wstr1, plate);
				sheet->writeStr(index + 1, 0, wstr1.c_str());
				StringToWstring(wstr2, color);
				sheet->writeStr(index + 1, 1, wstr2.c_str());
				//int rows = sheet->lastRow();
				//int cols = sheet->lastCol();
				//std::cout << rows << "  " << cols << std::endl;
				//for (int i = 0; i < files.size(); i++) {

				//	std::string str = files[i];
				//	auto iter = str.begin();
				//	while (iter != str.end())
				//	{
				//		if (*iter == '\\')
				//		{
				//			*iter = '/';
				//		}
				//		iter++;
				//	}
				//	wstring wstr1;
				//	StringToWstring(wstr1, str);
				//	sheet->writeStr(i + 1, 2, wstr1.c_str());
				//}
			}
			if (book->save(path))
			{
				std::cout << "File example.xls has been modified." << std::endl;
				//std::string path1;
				//Wchar_tToString(path1, path);
				//::ShellExecute(NULL, "open", path1.c_str(), NULL, NULL, SW_SHOW);
			}
			else
			{
				std::cout << book->errorMessage() << std::endl;
			}
		}
		else
		{
			std::cout << "*.xlsx file path is wrong !" << std::endl;
		}

		book->release();
	}
}