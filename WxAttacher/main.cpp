#include "WxAttacher.h"
#include <iostream>
#include <tlhelp32.h>

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

/**
 * @brief �رմ�����
 * @param fdwCtrlType
 * @return
*/
BOOL WINAPI ctrlHandler(DWORD pid, DWORD fdwCtrlType)
{
	if (fdwCtrlType == CTRL_CLOSE_EVENT) {
		char szbuf[0x1000] = { 0 };
		setvbuf(stdout, szbuf, _IOFBF, 0x1000);

		return TRUE;
	}
	return FALSE;
}

std::string a2utf8(const std::string& source)
{
	unsigned long len = ::MultiByteToWideChar(GetConsoleCP(), NULL, source.c_str(), -1, NULL, NULL);
	if (len == 0)
		return std::string();
	wchar_t* wide_char_buffer = new wchar_t[len];
	::MultiByteToWideChar(GetConsoleCP(), NULL, source.c_str(), -1, wide_char_buffer, len);

	len = ::WideCharToMultiByte(CP_UTF8, NULL, wide_char_buffer, -1, NULL, NULL, NULL, NULL);
	if (len == 0)
	{
		delete[] wide_char_buffer;
		return std::string();
	}
	char* multi_byte_buffer = new char[len];
	::WideCharToMultiByte(CP_UTF8, NULL, wide_char_buffer, -1, multi_byte_buffer, len, NULL, NULL);

	std::string dest(multi_byte_buffer);
	delete[] wide_char_buffer;
	delete[] multi_byte_buffer;
	return dest;
}

/**
 * @brief ���������˳�
 * @param pid
 * @return
*/
DWORD WINAPI daemonExit(PVOID pid) {
	HANDLE hProcessCmd = ::OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);

	if (hProcessCmd)
		WaitForSingleObject(hProcessCmd, INFINITE);

	DWORD dwCode = 0;
	GetExitCodeProcess(hProcessCmd, &dwCode);
	CloseHandle(hProcessCmd);

	SLOG_INFO(L"�����������˳� - [pid:{}, exitCode:{}]", (DWORD)pid, dwCode);

	detachWxCollector();
	PostQuitMessage(0);

	Sleep(500);
	exit(0);

	return 0;
}

/**
 * @brief ��ָ��������ִ��
 * @param pid
 * @param pName
 * @param lpStartAddress
 * @return
*/
HMODULE execProcessFun(DWORD pid, LPCWSTR pName, LPTHREAD_START_ROUTINE lpStartAddress) {
	HANDLE hSnap = INVALID_HANDLE_VALUE;

	PROCESSENTRY32 pe32 = { sizeof(PROCESSENTRY32) };
	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, pid);

	//��������Ч�ͷ���
	if (hSnap == INVALID_HANDLE_VALUE) {
		return 0;
	}

	if (!Process32First(hSnap, &pe32)) {
		//��ȡʧ����رվ��
		CloseHandle(hSnap);
		return 0;
	}

	do
	{
		if (lstrcmp(pe32.szExeFile, pName) == 0)
		{
			CreateThread(NULL, 0, lpStartAddress, (LPVOID)pe32.th32ProcessID, 0, NULL);
			CloseHandle(hSnap);
			break;
		}

	} while (Process32First(hSnap, &pe32));

	return 0;
}

/**
 * @brief ��������
*/
std::map<std::wstring, std::wstring> startupOptions;

int main(int argc, char* argv[]) {
	openConsole();

	initEnv();

	if (findWxAttacherWindow()) {
		MessageBox(NULL, L"��һ��ʵ���Ѿ�������", L"����", MB_OK);
		SLOG_ERROR(L"��һ��ʵ���Ѿ�������");
		return -1;
	}

	if (argc < 2) {
		MessageBox(NULL, L"������������", L"����", MB_OK);
		SLOG_ERROR(L"������������");
		return -1;
	}

	// ��������
	for (int i = 1; i < argc; i++) {
		std::vector<std::wstring> kv = Strs::split(Strs::s2ws(argv[i]), L"=");

		//SLOG_DEBUG(L"{}::{}:{}", i, kv[0], kv[1]);
		startupOptions[kv[0]] = (kv.size() == 2 ? kv[1] : L"");
	}

	loadConf();

	auto token = startupOptions.find(L"-t");
	if (token == startupOptions.end() || !Strs::len(token->second)) {
		MessageBox(NULL, L"tokenȱʧ", L"����", MB_OK);
		return -1;
	}
	else {
		// token
		swprintf_s(clientLoginInfo.token, token->second.data());
	}

	auto listenPid = startupOptions.find(L"-l");
	if (listenPid != startupOptions.end() || Strs::len(listenPid->second)) {
		SLOG_INFO(L"�������̼��� - [pid:{}]", listenPid->second);
		std::thread(daemonExit, (PVOID)(_wtoi(listenPid->second.data()))).detach();
	}

	std::thread(init, GetModuleHandle(NULL)).detach();
	Sleep(500);

	// ����̨�˳�����
	//SetConsoleCtrlHandler(ctrlHandler, TRUE);

	std::cout << u8"[ \
	attach:����DLL \
	detach:ж��DLL \
	close:�ر�WX_ATTACHER&WX_COLLECCTOR \
	openattacherconsole:��ATTACHER���Կ���̨ \
	closeattacherconsole:�ر�ATTACHER���Կ���̨ \
	opencollectorconsole:��COLLECTOR���Կ���̨ \
	closecollectorconsole:�ر�COLLECTOR���Կ���̨ \
	follow:���ڸ��� \
	unfollow:ȡ�����ڸ��� \
	getcontact:��ȡ��ϵ���б� \
	synccontact:ͬ����ϵ���б� \
	sendmsg:�����ı���Ϣ \
	]" << std::endl;

	std::string in;
	while (1) {
		std::cout << u8">> ";
		std::cin >> in;
		in = a2utf8(in);
		std::cout << in << std::endl;

		if (in == u8"attach") {
			attachWxCollector(L"");
		}
		else if (in == u8"detach") {
			detachWxCollector();
		}
		else if (in == u8"close") {
			detachWxCollector();
			Sleep(500);
			exit(0);
		}
		else if (in == u8"openattacherconsole") {
			//openConsole();
			HWND hWnd = findWxAttacherDebugConsoleWindow();
			ShowWindow(hWnd, SW_SHOW);
			UpdateWindow(hWnd);
		}
		else if (in == u8"closeattacherconsole") {
			//freeConsole();
			HWND hWnd = findWxAttacherDebugConsoleWindow();
			ShowWindow(hWnd, SW_HIDE);
			UpdateWindow(hWnd);
		}
		else if (in == u8"opencollectorconsole") {
			sendWmMsg(WM_COPYDATA_EVENT_OPEN_COLLECTOR_CONSOLE, 0, NULL);
		}
		else if (in == u8"closecollectorconsole") {
			sendWmMsg(WM_COPYDATA_EVENT_CLOSE_COLLECTOR_CONSOLE, 0, NULL);
		}
		else if (in == u8"follow") {
			HWND hw1 = FindWindow(TEXT("WeChatMainWndForPC"), NULL);
			HWND hw2 = FindWindow(TEXT("ConsoleWindowClass"), NULL);
			std::thread(follWnd, hw1, hw2).detach();
		}
		else if (in == u8"unfollow") {
			extern bool FOLL_WND;
			FOLL_WND = false;
		}
		else if (in == u8"getcontact") {
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_PULL_REQ, 0, NULL);
		}
		else if (in == u8"synccontact") {
			// ��ϵ��ͬ��
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_SYNC_REQ, 0, NULL);
		}
	}

	return 0;
}
