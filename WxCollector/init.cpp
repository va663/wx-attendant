#include "pch.h"
#include <strstream>
#include <sstream>
#include <iostream>
#include <map>

#pragma warning(disable:4996)
#pragma comment(lib,"Version.lib")

HMODULE WX_WECHATWINDLL_BASE_ADDR = 0;

std::wstring ENV_EXE_PATH;
std::wstring ENV_EXE_ABSOLUTE_DIR;

std::wstring ENV_MOUDLE_PATH;
std::wstring ENV_MOUDLE_ABSOLUTE_DIR;
std::wstring ENV_MOUDLE_FILENAME;
BOOL ENV_MODULE_DETACHED = FALSE;
HMODULE ENV_MOUDLE_BASE_ADDR = 0;

/**
 * @brief ���汻hook��ַ��ԭ�������ڻָ�
*/
std::map<DWORD, std::pair<LPVOID, UINT>> hookRecoveryMap;

/**
 * @brief ��ȡWeChatWin��ַ WeChatWin.dll
 * @return
*/
DWORD getWeChatWinDllBase()
{
	//ֻ��ȡһ��
	return (DWORD)(WX_WECHATWINDLL_BASE_ADDR ? WX_WECHATWINDLL_BASE_ADDR : WX_WECHATWINDLL_BASE_ADDR = GetModuleHandle(TEXT(WX_WECHATWINDLL)));
}

/**
 * @brief ��ʼ����ǰDLLģ����ػ�������
 * @param hModule
*/
void initModuleEnv(HMODULE hModule) {

	WCHAR exePath[MAX_PATH];
	GetModuleFileName(NULL, exePath, MAX_PATH);

	ENV_EXE_PATH = exePath;
	ENV_EXE_ABSOLUTE_DIR = Files::getFileAbsoluteDir(exePath);

	WCHAR collectorDllPath[MAX_PATH];
	GetModuleFileName(hModule, collectorDllPath, MAX_PATH);

	ENV_MOUDLE_PATH = collectorDllPath;
	ENV_MOUDLE_ABSOLUTE_DIR = Files::getFileAbsoluteDir(collectorDllPath);
	ENV_MOUDLE_FILENAME = Files::getFilename(collectorDllPath);
	ENV_MOUDLE_BASE_ADDR = hModule;

	SLOG_INFO(L"��ʼ������, ENV_EXE_PATH:[{}], ENV_MOUDLE_PATH:[{}]",
		ENV_EXE_PATH, ENV_MOUDLE_PATH);
}

/**
 * @brief �򿪿���̨
*/
void openConsole() {
	AllocConsole();
	SetConsoleOutputCP(65001);
	LPCWSTR title = L"WxCollectorDebugConsole";
	SetConsoleTitle(title);
	Sleep(300);
	RemoveMenu(GetSystemMenu(FindWindow(NULL, title), FALSE), 0xF060, FALSE);
}

/**
 * @brief �رտ���̨
*/
void freeConsole() {
	FreeConsole();
}

/**
 * @brief ���������̨
 * @param str
*/
void writeConsole(std::wstring str) {
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str.data(), str.length() * sizeof(WCHAR), NULL, NULL);
}

/**
 * @brief ��ȡWX_ATTACHER HWND
 * @return
*/
HWND findWxAttacherWindow() {
	return FindWindow(TEXT("WX_ATTACHER"), TEXT("WX_ATTACHER"));
}

/**
 * @brief ����WM_COPYDATA��Ϣ
 * @param wmEvent
 * @param size
 * @param payload
*/
void sendWmMsg(DWORD wmEvent, DWORD size, PVOID payload)
{
	// ���͵�attacher
	HWND hWnd = findWxAttacherWindow();
	if (hWnd == NULL)
	{
		SLOG_ERROR(u8"WM��Ϣ����ʧ��[δ�ҵ�WX_ATTACHER], wmEvent:[{}], paylaod:[{}]", wmEvent, payload);
		return;
	}

	COPYDATASTRUCT data;
	data.dwData = wmEvent;
	data.cbData = size;
	data.lpData = payload;

	SendMessage(hWnd, WM_COPYDATA, (DWORD)GetModuleHandle(NULL), (LPARAM)&data);
}

/**
 * @brief ����WM_COPYDATA֪ͨ��Ϣ
 * @param msg
*/
void sendWmNoticeMsg(LPCWSTR msg) {
	sendWmMsg(WM_COPYDATA_EVENT_NOTICE, (Strs::len(msg) + 1) * sizeof(WCHAR), (PVOID)msg);
}

/**
 * @brief hook�����ַ
 * @param dwHookAddr ��ҪHOOK�ĵ�ַ
 * @param dwJmpAddress hook������ַ
*/
void hookAnyAddress(DWORD dwHookAddr, LPVOID dwJmpAddress)
{
	if (dwHookAddr == NULL || dwJmpAddress == NULL) {
		MessageBox(NULL, L"HOOK addr is NULL", L"����", MB_OK);
		return;
	}

	// ���ݳ���
	const int len = 5;

	// ������������
	BYTE* recoveryData = new BYTE[len]();

	//��װ��ת����
	BYTE jmpCode[len] = { 0 };
	jmpCode[0] = 0xE9;

	//����ƫ��
	*(DWORD*)&jmpCode[1] = (DWORD)dwJmpAddress - dwHookAddr - len;

	// ������ǰ���������ڻ�ԭ
	DWORD OldProtext = 0;

	// ��ΪҪ�������д�����ݣ�����Ϊ������ǲ���д�ģ�������Ҫ�޸�����
	VirtualProtect((LPVOID)dwHookAddr, len, PAGE_EXECUTE_READWRITE, &OldProtext);

	// ��������
	memcpy(recoveryData, (void*)dwHookAddr, len);

	// д���Լ��Ĵ���
	memcpy((void*)dwHookAddr, jmpCode, len);

	// ִ�����˲���֮����Ҫ���л�ԭ
	VirtualProtect((LPVOID)dwHookAddr, len, OldProtext, &OldProtext);

	// ����ԭ�����Թ���ԭ
	hookRecoveryMap.insert(std::make_pair(dwHookAddr, std::pair<LPVOID, UINT>(recoveryData, len)));
}

/**
 * @brief hook��ԭ
 * @param dwHookAddr
*/
void hookRecovery(DWORD dwHookAddr)
{
	auto i = hookRecoveryMap.find(dwHookAddr);
	if (i != hookRecoveryMap.end()) {
		// ������ǰ���������ڻ�ԭ
		DWORD OldProtext = 0;

		// ��ΪҪ�������д�����ݣ�����Ϊ������ǲ���д�ģ�������Ҫ�޸�����
		VirtualProtect((LPVOID)dwHookAddr, i->second.second, PAGE_EXECUTE_READWRITE, &OldProtext);

		// д���Լ��Ĵ���
		memcpy((void*)dwHookAddr, i->second.first, i->second.second);

		// ִ�����˲���֮����Ҫ���л�ԭ
		VirtualProtect((LPVOID)dwHookAddr, i->second.second, OldProtext, &OldProtext);

		// ����
		delete[] i->second.first;
		hookRecoveryMap.erase(i);
	}
}

/**
 * @brief hook��ԭ����
*/
void hookRecoveryAll()
{
	for (const auto& i : hookRecoveryMap) {
		hookRecovery(i.first);
	}
}

/**
 * @brief ������Ϣ�ص�����
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 * @return
*/
LRESULT CALLBACK wndProc(
	_In_  HWND hwnd,
	_In_  UINT message,
	_In_  WPARAM wParam,
	_In_  LPARAM lParam
) {
	if (message == WM_COPYDATA) {
		receiveWmMsg((COPYDATASTRUCT*)lParam);
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

/**
 * @brief ��Ϣѭ��
*/
void msgLoop() {
	SLOG_INFO(u8"��Ϣѭ������");

	MSG msg = { 0 };

	while (BOOL flag = GetMessage(&msg, NULL, 0, 0)) {
		//SLOG_INFO(L"��Ϣѭ��:{}", flag);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	SLOG_INFO(u8"��Ϣѭ���˳�");
}

/**
 * @brief ע��һ���������ڽ��̼�ͨ��
 * @param hModule
 * @param lpClassName
 * @param lpWindowName
 * @param lpfnWndProc
*/
void registerWindow(HMODULE hModule, LPCWSTR lpClassName, LPCWSTR lpWindowName, WNDPROC lpfnWndProc) {

	// ������
	WNDCLASS wndClass;
	SecureZeroMemory(&wndClass, sizeof(WNDCLASS));
	wndClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = lpfnWndProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hModule;
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = lpClassName;

	// ע�ᴰ����
	RegisterClass(&wndClass);

	// ��������
	HWND hWnd = CreateWindow(
		lpClassName, // ��������
		lpWindowName, //������
		WS_OVERLAPPEDWINDOW, // ���ڷ��
		1, 1,             // ����λ��
		0, 0,				// ���ڴ�С
		NULL,             //�����ھ��
		NULL,             //�˵����
		hModule,        //ʵ�����
		NULL              //����WM_CREATE��Ϣʱ�ĸ��Ӳ���
	);

	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	msgLoop();
}

/**
 * @brief ��ȡ�汾��
 * @param hModule
 * @return
*/
std::string getVersion(HMODULE hModule)
{
	WCHAR VersionFilePath[MAX_PATH];
	if (!hModule || GetModuleFileName(hModule, VersionFilePath, MAX_PATH) == 0)
	{
		return "";
	}

	std::stringstream verStream;

	VS_FIXEDFILEINFO* pVsInfo;
	unsigned int iFileInfoSize = sizeof(VS_FIXEDFILEINFO);
	int iVerInfoSize = GetFileVersionInfoSize(VersionFilePath, NULL);
	if (iVerInfoSize != 0) {
		char* pBuf = new char[iVerInfoSize];
		memset(pBuf, 0, iVerInfoSize);
		if (GetFileVersionInfo(VersionFilePath, 0, iVerInfoSize, pBuf)) {
			if (VerQueryValue(pBuf, TEXT("\\"), (void**)&pVsInfo, &iFileInfoSize)) {

				int s_major_ver = (pVsInfo->dwFileVersionMS >> 16) & 0x0000FFFF;

				int s_minor_ver = pVsInfo->dwFileVersionMS & 0x0000FFFF;

				int s_build_num = (pVsInfo->dwFileVersionLS >> 16) & 0x0000FFFF;

				int s_revision_num = pVsInfo->dwFileVersionLS & 0x0000FFFF;

				//�Ѱ汾����ַ���
				verStream << s_major_ver << "." << s_minor_ver << "." << s_build_num << "." << s_revision_num;
			}
		}
		delete[] pBuf;
	}

	return verStream.str();
}

/**
 * @brief ���΢�Ű汾
 * @return
*/
bool checkWxVersion()
{
	const std::string wxVersoin = WX_VERSION;
	std::string v = getVersion((HMODULE)getWeChatWinDllBase());
	SLOG_INFO(u8"΢�Ű汾:{}, �Ƿ�֧��:{}", v, v == wxVersoin);
	return v == wxVersoin;
}

/**
 * @brief ����hook����
*/
void hook() {
	hookLogout();
	hookReceiveMsg();
	hookChatContactSwitch();
}

/**
 * @brief ��ʼ��
 * @param hModule
*/
void init(HMODULE hModule)
{
	//��鵱ǰ΢�Ű汾
	if (!checkWxVersion())
	{
		std::wstring v = Strs::s2ws(getVersion((HMODULE)getWeChatWinDllBase()));
		std::wstring sv = TEXT(WX_VERSION);
		v.append(L"|").append(sv);
		sendWmMsg(WM_COPYDATA_EVENT_WX_VERSION_NOT_SUPPORT, (Strs::len(v) + 1) * sizeof(WCHAR), (PVOID)v.data());

		//std::wstring errMsg;
		//errMsg.append(L"��ǰ΢�Ű汾��֧�֣�������").append(TEXT(WX_VERSION)).append(L"�汾");

		//MessageBox(NULL, errMsg.c_str(), L"����", MB_OK);

		return;
	}

	if (getWeChatWinDllBase()) {
		// ע�ᴰ��
		std::thread(registerWindow, hModule, L"WX_COLLECTOR", L"WX_COLLECTOR", wndProc).detach();

		// ��ȡ��¼�û���Ϣ
		do {
			if (loggedin()) {
				SLOG_INFO(L"΢�ŵ�¼�û���Ϣ��ȡ��");

				WxLoginInfo loginInfo = getLoginInfo();
				if (Strs::len(loginInfo.wxid)) {
					WCHAR msg[1024];
					wsprintf(msg, L"΢�ŵ�¼�û� - [wxid:%s, wxcid:%s, phone:%s, wxname:%s, avatar:%s]", loginInfo.wxid, loginInfo.wxcid, loginInfo.phone, loginInfo.wxname, loginInfo.avatar);
					SLOG_INFO(msg);

					// ���hook
					hook();

					// ���ͳ�ʼ���ɹ���Ϣ
					sendWmMsg(WM_COPYDATA_EVENT_COLLECTOR_INIT, sizeof(WxLoginInfo), (PVOID)&loginInfo);
					break;
				}
			}
			else {
				SLOG_INFO(L"΢��δ��¼");
			}

			Sleep(500);
		} while (!ENV_MODULE_DETACHED);

	}

}
