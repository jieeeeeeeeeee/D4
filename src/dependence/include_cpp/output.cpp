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
				//std::cout << "begin!" << std::endl;
				////double d = sheet->readNum(4, 1);
				//double  xx = sheet->readNum(0, 0);
				//if (xx) std::cout << xx << endl;
				//const wchar_t* s = sheet->readStr(0, 0);
				//if (s) std::wcout << s << std::endl << std::endl;

				//sheet->writeNum(0, 0, 44);
				//sheet->writeStr(4, 0, L"new string !");
				int rows = sheet->lastRow();
				int cols = sheet->lastCol();
				std::cout << rows << "  " << cols << std::endl;
				for (int i = 1; i < rows; i++) {
					const wchar_t* wstr = sheet->readStr(i, 2);
					//if (wstr) std::wcout << wstr << std::endl << std::endl;
					//wchar_t* wstr = L"img_test\功能评测图像库\省市简称变化子库\“云”牌\云A00290.jpg";
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
					//wstring wstr1;
					//StringToWstring(wstr1, str);
					//sheet->writeStr(i, 3, wstr1.c_str());

				}
			}
			if (book->save(path))
			{
				std::cout << "\nFile example.xls has been modified." << std::endl;
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
	for (int i = 0; i < files.size(); i++) {
		std::cout << files[i] << std::endl;
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
				//	sheet->writeStr(i+1, 2, wstr1.c_str());
				//}
			}
			if (book->save(path))
			{
				std::cout << "\nFile example.xls has been modified." << std::endl;
				//::ShellExecute(NULL, L"open", path, NULL, NULL, SW_SHOW);
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