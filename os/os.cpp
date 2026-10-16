// os.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <Windows.h>
#include"sync.h"
#include"macro.h"


int wmain(int argc, wchar_t* argv[])
{
	setlocale(LC_ALL, "UTF-8");

	SetConsoleOutputCP(CP_UTF8);
	SetConsoleCP(CP_UTF8);

	A_START;
	std::cout << "🌍测试" << std::endl;
	A_STOP;

	W_START;
	if (argc != 2 || _wcsicmp(argv[1], L"sync") != 0)
	{
		std::wcout << L"用法：os sync\n";
		return 1;
	}

	try
	{
		sync();
		return 0;
	}
	catch (const std::exception& e)
	{
		A_START;
		std::cerr << "Error: " << e.what() << '\n';
		A_STOP;
		return 1;
	}
	W_STOP;
}

