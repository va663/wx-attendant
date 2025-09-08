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
 * @brief 保存被hook地址的原数据用于恢复
*/
std::map<DWORD, std::pair<LPVOID, UINT>> hookRecoveryMap;

/**
 * @brief 获取WeChatWin基址 WeChatWin.dll
 * @return
*/
DWORD getWeChatWinDllBase()
{
	//只获取一次
	return (DWORD)(WX_WECHATWINDLL_BASE_ADDR ? WX_WECHATWINDLL_BASE_ADDR : WX_WECHATWINDLL_BASE_ADDR = GetModuleHandle(TEXT(WX_WECHATWINDLL)));
}

/**
 * @brief 初始化当前DLL模块相关环境属性
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

	SLOG_INFO(L"初始化环境, ENV_EXE_PATH:[{}], ENV_MOUDLE_PATH:[{}]",
		ENV_EXE_PATH, ENV_MOUDLE_PATH);
}

/**
 * @brief 打开控制台
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
 * @brief 关闭控制台
*/
void freeConsole() {
	FreeConsole();
}

/**
 * @brief 输出到控制台
 * @param str
*/
void writeConsole(std::wstring str) {
	WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE), str.data(), str.length() * sizeof(WCHAR), NULL, NULL);
}

/**
 * @brief 获取WX_ATTACHER HWND
 * @return
*/
HWND findWxAttacherWindow() {
	return FindWindow(TEXT("WX_ATTACHER"), TEXT("WX_ATTACHER"));
}

/**
 * @brief 发送WM_COPYDATA消息
 * @param wmEvent
 * @param size
 * @param payload
*/
void sendWmMsg(DWORD wmEvent, DWORD size, PVOID payload)
{
	// 发送到attacher
	HWND hWnd = findWxAttacherWindow();
	if (hWnd == NULL)
	{
		SLOG_ERROR(u8"WM消息发送失败[未找到WX_ATTACHER], wmEvent:[{}], paylaod:[{}]", wmEvent, payload);
		return;
	}

	COPYDATASTRUCT data;
	data.dwData = wmEvent;
	data.cbData = size;
	data.lpData = payload;

	SendMessage(hWnd, WM_COPYDATA, (DWORD)GetModuleHandle(NULL), (LPARAM)&data);
}

/**
 * @brief 发送WM_COPYDATA通知消息
 * @param msg
*/
void sendWmNoticeMsg(LPCWSTR msg) {
	sendWmMsg(WM_COPYDATA_EVENT_NOTICE, (Strs::len(msg) + 1) * sizeof(WCHAR), (PVOID)msg);
}

/**
 * @brief hook任意地址
 * @param dwHookAddr 需要HOOK的地址
 * @param dwJmpAddress hook函数地址
*/
void hookAnyAddress(DWORD dwHookAddr, LPVOID dwJmpAddress)
{
	if (dwHookAddr == NULL || dwJmpAddress == NULL) {
		MessageBox(NULL, L"HOOK addr is NULL", L"错误", MB_OK);
		return;
	}

	// 数据长度
	const int len = 5;

	// 备份数据数组
	BYTE* recoveryData = new BYTE[len]();

	//组装跳转数据
	BYTE jmpCode[len] = { 0 };
	jmpCode[0] = 0xE9;

	//计算偏移
	*(DWORD*)&jmpCode[1] = (DWORD)dwJmpAddress - dwHookAddr - len;

	// 保存以前的属性用于还原
	DWORD OldProtext = 0;

	// 因为要往代码段写入数据，又因为代码段是不可写的，所以需要修改属性
	VirtualProtect((LPVOID)dwHookAddr, len, PAGE_EXECUTE_READWRITE, &OldProtext);

	// 备份数据
	memcpy(recoveryData, (void*)dwHookAddr, len);

	// 写入自己的代码
	memcpy((void*)dwHookAddr, jmpCode, len);

	// 执行完了操作之后需要进行还原
	VirtualProtect((LPVOID)dwHookAddr, len, OldProtext, &OldProtext);

	// 保存原数据以供复原
	hookRecoveryMap.insert(std::make_pair(dwHookAddr, std::pair<LPVOID, UINT>(recoveryData, len)));
}

/**
 * @brief hook复原
 * @param dwHookAddr
*/
void hookRecovery(DWORD dwHookAddr)
{
	auto i = hookRecoveryMap.find(dwHookAddr);
	if (i != hookRecoveryMap.end()) {
		// 保存以前的属性用于还原
		DWORD OldProtext = 0;

		// 因为要往代码段写入数据，又因为代码段是不可写的，所以需要修改属性
		VirtualProtect((LPVOID)dwHookAddr, i->second.second, PAGE_EXECUTE_READWRITE, &OldProtext);

		// 写入自己的代码
		memcpy((void*)dwHookAddr, i->second.first, i->second.second);

		// 执行完了操作之后需要进行还原
		VirtualProtect((LPVOID)dwHookAddr, i->second.second, OldProtext, &OldProtext);

		// 清理
		delete[] i->second.first;
		hookRecoveryMap.erase(i);
	}
}

/**
 * @brief hook复原所有
*/
void hookRecoveryAll()
{
	for (const auto& i : hookRecoveryMap) {
		hookRecovery(i.first);
	}
}

/**
 * @brief 窗口消息回调函数
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
 * @brief 消息循环
*/
void msgLoop() {
	SLOG_INFO(u8"消息循环启动");

	MSG msg = { 0 };

	while (BOOL flag = GetMessage(&msg, NULL, 0, 0)) {
		//SLOG_INFO(L"消息循环:{}", flag);
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	SLOG_INFO(u8"消息循环退出");
}

/**
 * @brief 注册一个窗口用于进程间通信
 * @param hModule
 * @param lpClassName
 * @param lpWindowName
 * @param lpfnWndProc
*/
void registerWindow(HMODULE hModule, LPCWSTR lpClassName, LPCWSTR lpWindowName, WNDPROC lpfnWndProc) {

	// 窗口类
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

	// 注册窗口类
	RegisterClass(&wndClass);

	// 创建窗口
	HWND hWnd = CreateWindow(
		lpClassName, // 窗口类名
		lpWindowName, //窗口名
		WS_OVERLAPPEDWINDOW, // 窗口风格
		1, 1,             // 窗口位置
		0, 0,				// 窗口大小
		NULL,             //父窗口句柄
		NULL,             //菜单句柄
		hModule,        //实例句柄
		NULL              //传递WM_CREATE消息时的附加参数
	);

	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	msgLoop();
}

/**
 * @brief 获取版本号
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

				//把版本变成字符串
				verStream << s_major_ver << "." << s_minor_ver << "." << s_build_num << "." << s_revision_num;
			}
		}
		delete[] pBuf;
	}

	return verStream.str();
}

/**
 * @brief 检查微信版本
 * @return
*/
bool checkWxVersion()
{
	const std::string wxVersoin = WX_VERSION;
	std::string v = getVersion((HMODULE)getWeChatWinDllBase());
	SLOG_INFO(u8"微信版本:{}, 是否支持:{}", v, v == wxVersoin);
	return v == wxVersoin;
}

/**
 * @brief 加入hook函数
*/
void hook() {
	hookLogout();
	hookReceiveMsg();
	hookChatContactSwitch();
}

/**
 * @brief 初始化
 * @param hModule
*/
void init(HMODULE hModule)
{
	//检查当前微信版本
	if (!checkWxVersion())
	{
		std::wstring v = Strs::s2ws(getVersion((HMODULE)getWeChatWinDllBase()));
		std::wstring sv = TEXT(WX_VERSION);
		v.append(L"|").append(sv);
		sendWmMsg(WM_COPYDATA_EVENT_WX_VERSION_NOT_SUPPORT, (Strs::len(v) + 1) * sizeof(WCHAR), (PVOID)v.data());

		//std::wstring errMsg;
		//errMsg.append(L"当前微信版本不支持，请下载").append(TEXT(WX_VERSION)).append(L"版本");

		//MessageBox(NULL, errMsg.c_str(), L"错误", MB_OK);

		return;
	}

	if (getWeChatWinDllBase()) {
		// 注册窗口
		std::thread(registerWindow, hModule, L"WX_COLLECTOR", L"WX_COLLECTOR", wndProc).detach();

		// 获取登录用户信息
		do {
			if (loggedin()) {
				SLOG_INFO(L"微信登录用户信息获取中");

				WxLoginInfo loginInfo = getLoginInfo();
				if (Strs::len(loginInfo.wxid)) {
					WCHAR msg[1024];
					wsprintf(msg, L"微信登录用户 - [wxid:%s, wxcid:%s, phone:%s, wxname:%s, avatar:%s]", loginInfo.wxid, loginInfo.wxcid, loginInfo.phone, loginInfo.wxname, loginInfo.avatar);
					SLOG_INFO(msg);

					// 添加hook
					hook();

					// 发送初始化成功消息
					sendWmMsg(WM_COPYDATA_EVENT_COLLECTOR_INIT, sizeof(WxLoginInfo), (PVOID)&loginInfo);
					break;
				}
			}
			else {
				SLOG_INFO(L"微信未登录");
			}

			Sleep(500);
		} while (!ENV_MODULE_DETACHED);

	}

}
