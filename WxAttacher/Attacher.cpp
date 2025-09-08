#pragma warning(disable:4996)

#include "WxAttacher.h"

#include <tlhelp32.h>
#include <iostream>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi")

/**
 * @brief ͨ���������ҵ�����ID
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
 * @brief �ҵ�ָ��΢�ŵĽ���ID
 * @param wxid
 * @return
*/
DWORD findWxPID(std::wstring wxid) {
	std::vector<DWORD> pids = findPID(TEXT(WX_PROCESS_NAME));
	if (pids.empty()) {
		return 0;
	}

	// ��ʱֻ���ص�һ��
	return pids[0];
}

/**
 * @brief ��ȡģ���ַ
 * @param pid
 * @param mName
 * @return
*/
HMODULE getModule(DWORD pid, LPCWSTR mName) {
	HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
	//��ʼ��ģ����Ϣ�ṹ��
	MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
	//����ģ����� 1 �������� 2 ����ID
	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	//��������Ч�ͷ���
	if (hModuleSnap == INVALID_HANDLE_VALUE)
	{
		return 0;
	}
	//ͨ��ģ����վ����ȡ��һ��ģ�����Ϣ
	if (!Module32First(hModuleSnap, &me32))
	{
		//��ȡʧ����رվ��
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
 * @brief ����ظ�ע��
 * @param dwProcessid
 * @return
*/
BOOL checkReduplicatedAttach(DWORD pid, LPCWSTR dllName)
{
	return getModule(pid, dllName) > 0;
}

/**
 * @brief ע��
 * @param pid
 * @param dllPath
 * @return
*/
int attach(DWORD pid, LPCWSTR dllPath)
{
	if (!pid || !dllPath) {
		SLOG_ERROR(L"attach��������, pid:[{}], dllPath:[{}]", pid, dllPath);
		return 1;
	}

	if (_waccess(dllPath, 4))
	{
		SLOG_ERROR(L"�Ҳ���:{}", dllPath);
		return 1;
	}
	if (checkReduplicatedAttach(pid, Files::getFilename((LPWSTR)dllPath).c_str()))
	{
		SLOG_WARN(L"Reduplicate attach, pid:{}, dllName:{}, dllPath:{}", pid, Files::getFilename((LPWSTR)dllPath), dllPath);
		return 2;
	}

	// ��ȡ��ע����̵ľ��
	HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!hprocess) {
		SLOG_ERROR(u8"�޷���ȡָ�����̾��,pid��{}", pid);
		return 1;
	}

	// ��ȡLoadLibraryW��������ʼ��ַ
	PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "LoadLibraryW");
	if (!pfnStartAddress) {
		SLOG_ERROR(u8"�޷���ȡLoadLibraryW������ַ");
		return 1;
	}

	// �ڴ�ע�������Ϊ�̺߳�����LoadLibraryW���Ĳ�������̬���ӿ��·�������������ڴ�
	SIZE_T pathSize = (wcslen(dllPath) + 1) * sizeof(WCHAR);
	LPVOID dllPathAddress = VirtualAllocEx(hprocess, NULL, pathSize, MEM_COMMIT, PAGE_READWRITE);
	if (!dllPathAddress) {
		SLOG_ERROR(u8"����DLL·���ڴ�ʧ��");
		return 1;
	}

	// д��DLL·�����ڴ�
	if (!WriteProcessMemory(hprocess, dllPathAddress, dllPath, pathSize, NULL)) {
		SLOG_ERROR(u8"�޷�д��DLL·��");
		return 1;
	}

	// �ڴ�ע������д���Զ���̣߳�ע��DLL��
	HANDLE hThread = CreateRemoteThread(hprocess, NULL, NULL, pfnStartAddress, dllPathAddress, NULL, NULL);
	if (!hThread) {
		SLOG_ERROR(u8"����Զ���߳�ʧ��");
		return 1;
	}

	// �ȴ�Զ���߳̽������ȴ� 2000ms
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hprocess);

	return 0;
}
/**
 * @brief ��ע��
 * @param pid
 * @param dllPath
 * @return
*/
int detach(DWORD pid, LPCWSTR dllPath)
{
	if (!pid || !dllPath) {
		SLOG_ERROR(L"detach��������, pid:[{}], dllPath:[{}]", pid, dllPath);
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

	// ��ȡ����ע����̵ľ��
	HANDLE hprocess = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
	if (!hprocess) {
		SLOG_ERROR(u8"�޷���ȡָ�����̾��,pid��{}", pid);
		return 1;
	}

	// ��ȡFreeLibrary��������ʼ��ַ
	PTHREAD_START_ROUTINE pfnStartAddress = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "FreeLibrary");
	if (!pfnStartAddress) {
		SLOG_ERROR(u8"�޷���ȡFreeLibrary������ַ");
		return 1;
	}

	// �ڴ���ע������д���Զ���̣߳���ע��DLL��
	HANDLE hThread = CreateRemoteThread(hprocess, NULL, NULL, pfnStartAddress, hModule, NULL, NULL);
	if (!hThread) {
		SLOG_ERROR(u8"����Զ���߳�ʧ��");
		return 1;
	}

	// �ȴ�Զ���߳̽���
	WaitForSingleObject(hThread, INFINITE);

	CloseHandle(hThread);
	CloseHandle(hprocess);

	return 0;
}