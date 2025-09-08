#pragma warning(disable:4996)

#include "WxAttacher.h"

#include <tlhelp32.h>
#include <iostream>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

/**
 * @brief 通过进程名找到进程ID
 * @param ProcessName
 * @return
*/
std::vector<DWORD> findPID(LPCWSTR processName)
{
	std::vector<DWORD> pids;
	PROCESSENTRY32 pe32 = { 0 };
	pe32.dwSize = sizeof(PROCESSENTRY32);
	HANDLE hProcess = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (Process32First(hProcess, &pe32) == TRUE)
	{
		do
		{
			if (lstrcmp(processName, pe32.szExeFile) == 0)
			{
				pids.push_back(pe32.th32ProcessID);
			}
		} while (Process32Next(hProcess, &pe32));
	}
	return pids;
}

/**
 * @brief 找到指定微信的进程ID
 * @param wxid
 * @return
*/
DWORD findWxPID(std::wstring wxid) {
	std::vector<DWORD> pids = findPID(TEXT(WX_PROCESS_NAME));
	if (pids.empty()) {
		return 0;
	}

	// 暂时只返回第一个
	return pids[0];
}

/**
 * @brief 获取模块基址
 * @param pid
 * @param mName
 * @return
*/
HMODULE getModule(DWORD pid, LPCWSTR mName) {
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	//初始化模块信息结构体
	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
	//创建模块快照 1 快照类型 2 进程ID
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	//如果句柄无效就返回
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	//通过模块快照句柄获取第一个模块的信息
	if (!Module32First(hModuleSnap, &me32))
	{
		//获取失败则关闭句柄
		CloseHandle(hModuleSnap);
		return 0;
	}
	do
	{
		if (lstrcmp(me32.szModule, mName) == 0)
		{
			CloseHandle(hModuleSnap);
			return me32.hModule;
		}
	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);

	return 0;
}

/**
 * @brief 检查重复注入
 * @param dwProcessid
 * @return
*/
BOOL checkReduplicatedAttach(DWORD pid, LPCWSTR dllName)
{
	return getModule(pid, dllName) > 0;
}

/**
 * @brief 注入
 * @param pid
 * @param dllPath
 * @return
*/
int attach(DWORD pid, LPCWSTR dllPath)
{
	if (!pid || !dllPath) {
		SLOG_ERROR(L"attach参数错误, pid:[{}], dllPath:[{}]", pid, dllPath);
		return 1;
	}

	if (_waccess(dllPath, 4))
	{
		SLOG_ERROR(L"找不到:{}", dllPath);
		return 1;
	}
	if (checkReduplicatedAttach(pid, Files::getFilename((LPWSTR)dllPath).c_str()))
	{
		SLOG_WARN(L"Reduplicate attach, pid:{}, dllName:{}, dllPath:{}", pid, Files::getFilename((LPWSTR)dllPath), dllPath);
		return 2;
	}

	// 获取待注入进程的句柄
	HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!hprocess) {
		SLOG_ERROR(u8"无法获取指定进程句柄,pid：{}", pid);
		return 1;
	}

	// 获取LoadLibraryW函数的起始地址
	PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryW");
	if (!pfnStartAddress) {
		SLOG_ERROR(u8"无法获取LoadLibraryW函数地址");
		return 1;
	}

	// 在待注入进程中为线程函数（LoadLibraryW）的参数（动态链接库的路径）分配虚拟内存
	SIZE_T pathSize = (wcslen(dllPath) + 1) * sizeof(WCHAR);
	LPVOID dllPathAddress = VirtualAllocEx(hprocess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
	if (!dllPathAddress) {
		SLOG_ERROR(u8"开辟DLL路径内存失败");
		return 1;
	}

	// 写入DLL路径至内存
	if (!WriteProcessMemory(hprocess, dllPathAddress, dllPath, pathSize, NULL)) {
		SLOG_ERROR(u8"无法写入DLL路径");
		return 1;
	}

	// 在待注入进程中创建远程线程（注入DLL）
	HANDLE hThread = CreateRemoteThread(hprocess, NULL, NULL, pfnStartAddress, dllPathAddress, NULL, NULL);
	if (!hThread) {
		SLOG_ERROR(u8"创建远程线程失败");
		return 1;
	}

	// 等待远程线程结束，等待 2000ms
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hprocess);

	return 0;
}
/**
 * @brief 逆注入
 * @param pid
 * @param dllPath
 * @return
*/
int detach(DWORD pid, LPCWSTR dllPath)
{
	if (!pid || !dllPath) {
		SLOG_ERROR(L"detach参数错误, pid:[{}], dllPath:[{}]", pid, dllPath);
		return 1;
	}

	std::wstring dllName = Files::getFilename((LPWSTR)dllPath);
	HMODULE hModule = getModule(pid, dllName.c_str());
	if (hModule) {
		SLOG_INFO(L"found {}", dllName);
	}
	else {
		SLOG_INFO(L"not found {}", dllName);
		return 1;
	}

	// 获取待逆注入进程的句柄
	HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!hprocess) {
		SLOG_ERROR(u8"无法获取指定进程句柄,pid：{}", pid);
		return 1;
	}

	// 获取FreeLibrary函数的起始地址
	PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "FreeLibrary");
	if (!pfnStartAddress) {
		SLOG_ERROR(u8"无法获取FreeLibrary函数地址");
		return 1;
	}

	// 在待逆注入进程中创建远程线程（逆注入DLL）
	HANDLE hThread = CreateRemoteThread(hprocess, NULL, NULL, pfnStartAddress, hModule, NULL, NULL);
	if (!hThread) {
		SLOG_ERROR(u8"创建远程线程失败");
		return 1;
	}

	// 等待远程线程结束
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hprocess);

	return 0;
}