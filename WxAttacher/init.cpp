#include <strstream>
#include <iostream>
#include <future>
#include <thread>

#include "WebSocket.h"
#include "WxAttacher.h"
#include "Json.h"

#pragma warning(disable:4996)
#pragma comment(lib,"Version.lib")

/////////////////////////////////////////////
// ȫ�ֱ���

std::wstring ENV_EXE_PATH;
std::wstring ENV_EXE_ABSOLUTE_DIR;

std::wstring ENV_WX_COLLECTOR_DLL_PATH;
std::wstring ENV_WX_COLLECTOR_ABSOLUTE_DIR;

std::wstring ENV_CONF_LOCAL_WS_HOST;
int ENV_CONF_LOCAL_WS_PORT;
std::wstring ENV_CONF_LOCAL_WS_PATH;
bool ENV_CONF_LOCAL_WS_SSL;
std::wstring ENV_CONF_LOCAL_WS_ENCRYPT;

std::wstring ENV_CONF_REMOTE_WS_HOST;
int ENV_CONF_REMOTE_WS_PORT;
std::wstring ENV_CONF_REMOTE_WS_PATH;
bool ENV_CONF_REMOTE_WS_SSL;
std::wstring ENV_CONF_REMOTE_WS_ENCRYPT;

////////////////////////////////////////////


///////////////////////////////
WsSession* localWs = nullptr;

WsSession* remoteWs = nullptr;

bool FOLL_WND = false;

ClientLoginInfo clientLoginInfo = { 0 };
WxLoginInfo wxLoginInfo = { 0 };
//////////////////////////////

/**
 * @brief ������ʼ��
*/
void initEnv()
{
	WCHAR exePath[MAX_PATH];
	GetModuleFileName(NULL, exePath, MAX_PATH);

	ENV_EXE_PATH = exePath;
	ENV_EXE_ABSOLUTE_DIR = Files::getFileAbsoluteDir(exePath);

	ENV_WX_COLLECTOR_ABSOLUTE_DIR = ENV_EXE_ABSOLUTE_DIR;
	ENV_WX_COLLECTOR_DLL_PATH = ENV_WX_COLLECTOR_ABSOLUTE_DIR + (TEXT(WX_COLLECTOR_DLL_NAME));

	SLOG_INFO(L"��ʼ������, version:[{}], ENV_EXE_PATH:[{}], WX_PROCESS_NAME:[{}], ENV_WX_COLLECTOR_DLL_PATH:[{}]",
		Strs::s2ws(getVersion(GetModuleHandle(NULL))), ENV_EXE_PATH, TEXT(WX_PROCESS_NAME), ENV_WX_COLLECTOR_DLL_PATH);
}

/**
 * @brief �������ã������в�������
*/
void loadConf()
{
	std::wstring configFilePath = ENV_EXE_ABSOLUTE_DIR + TEXT("./conf.ini");
	WCHAR configBuf[512];

	// local ws conf
	auto localWsHostArg = startupOptions.find(TEXT("-localWsHost"));
	if (localWsHostArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("localWsHost"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_LOCAL_WS_HOST = configBuf;
	}
	else {
		ENV_CONF_LOCAL_WS_HOST = localWsHostArg->second;
	}

	auto localWsPortArg = startupOptions.find(TEXT("-localWsPort"));
	if (localWsPortArg == startupOptions.end()) {
		ENV_CONF_LOCAL_WS_PORT = GetPrivateProfileInt(TEXT("default"), TEXT("localWsPort"), 0, configFilePath.data());
	}
	else {
		ENV_CONF_LOCAL_WS_PORT = _wtoi(localWsPortArg->second.data());
	}

	auto localWsPathArg = startupOptions.find(TEXT("-localWsPath"));
	if (localWsPathArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("localWsPath"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_LOCAL_WS_PATH = configBuf;
	}
	else {
		ENV_CONF_LOCAL_WS_PATH = localWsPathArg->second;
	}

	auto localWsSSLArg = startupOptions.find(TEXT("-localWsSSL"));
	if (localWsSSLArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("localWsSSL"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_LOCAL_WS_SSL = Strs::equalsIgnoreCase(configBuf, L"true");;
	}
	else {
		ENV_CONF_LOCAL_WS_SSL = Strs::equalsIgnoreCase(localWsSSLArg->second, L"true");;
	}

	auto localWsEncryptArg = startupOptions.find(TEXT("-localWsEncrypt"));
	if (localWsEncryptArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("localWsEncrypt"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_LOCAL_WS_ENCRYPT = configBuf;
	}
	else {
		ENV_CONF_LOCAL_WS_ENCRYPT = localWsEncryptArg->second;
	}

	// remote ws conf
	auto remoteWsHostArg = startupOptions.find(TEXT("-remoteWsHost"));
	if (remoteWsHostArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("remoteWsHost"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_REMOTE_WS_HOST = configBuf;
	}
	else {
		ENV_CONF_REMOTE_WS_HOST = remoteWsHostArg->second;
	}

	auto remoteWsPortArg = startupOptions.find(TEXT("-remoteWsPort"));
	if (remoteWsPortArg == startupOptions.end()) {
		ENV_CONF_REMOTE_WS_PORT = GetPrivateProfileInt(TEXT("default"), TEXT("remoteWsPort"), 0, configFilePath.data());
	}
	else {
		ENV_CONF_REMOTE_WS_PORT = _wtoi(remoteWsPortArg->second.data());
	}

	auto remoteWsPathArg = startupOptions.find(TEXT("-remoteWsPath"));
	if (remoteWsPathArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("remoteWsPath"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_REMOTE_WS_PATH = configBuf;
	}
	else {
		ENV_CONF_REMOTE_WS_PATH = remoteWsPathArg->second;
	}

	auto remoteWsSSLArg = startupOptions.find(TEXT("-remoteWsSSL"));
	if (remoteWsSSLArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("remoteWsSSL"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_REMOTE_WS_SSL = Strs::equalsIgnoreCase(configBuf, L"true");
	}
	else {
		ENV_CONF_REMOTE_WS_SSL = Strs::equalsIgnoreCase(remoteWsSSLArg->second, L"true");;
	}

	auto remoteWsEncryptArg = startupOptions.find(TEXT("-remoteWsEncrypt"));
	if (remoteWsEncryptArg == startupOptions.end()) {
		GetPrivateProfileString(TEXT("default"), TEXT("remoteWsEncrypt"), TEXT(""), configBuf, 512, configFilePath.data());
		ENV_CONF_REMOTE_WS_ENCRYPT = configBuf;
	}
	else {
		ENV_CONF_REMOTE_WS_ENCRYPT = remoteWsEncryptArg->second;
	}

	SLOG_INFO(L"����������Ϣ, ENV_CONF_LOCAL_WS_URL:[{}:{}{} ssl:{} encrypt:{}], ENV_CONF_REMOTE_WS_URL:[{}:{}{} ssl:{} encrypt:{}]",
		ENV_CONF_LOCAL_WS_HOST, ENV_CONF_LOCAL_WS_PORT, ENV_CONF_LOCAL_WS_PATH, ENV_CONF_LOCAL_WS_SSL, ENV_CONF_LOCAL_WS_ENCRYPT,
		ENV_CONF_REMOTE_WS_HOST, ENV_CONF_REMOTE_WS_PORT, ENV_CONF_REMOTE_WS_PATH, ENV_CONF_REMOTE_WS_SSL, ENV_CONF_REMOTE_WS_ENCRYPT);
}

/**
 * @brief �򿪿���̨
*/
void openConsole() {
	AllocConsole();
	HWND hWnd = FindWindow(TEXT("ConsoleWindowClass"), NULL);
	ShowWindow(hWnd, SW_HIDE);
	UpdateWindow(hWnd);

	SetConsoleOutputCP(65001);
	SetConsoleCtrlHandler(NULL, true);
	Sleep(300);

	freopen("CONOUT$", "w+t", stdout);
	freopen("CONIN$", "r+t", stdin);

	LPCWSTR title = L"WxAttacherDebugConsole";
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
 * @brief ��ȡWX_ATTACHER HWND
 * @return
*/
HWND findWxAttacherWindow() {
	return FindWindow(TEXT("WX_ATTACHER"), TEXT("WX_ATTACHER"));
}

/**
 * @brief ��ȡWX_ATTACHER debug console HWND
 * @return
*/
HWND findWxAttacherDebugConsoleWindow() {
	return FindWindow(TEXT("ConsoleWindowClass"), TEXT("WxAttacherDebugConsole"));
}

/**
 * @brief ��ȡWX_COLLECTOR HWND
 * @return
*/
HWND findWxCollectorWindow() {
	return FindWindow(TEXT("WX_COLLECTOR"), TEXT("WX_COLLECTOR"));
}

/**
 * @brief ���ڸ��� hWnd2����hWnd1
 * @param hWnd1
 * @param hWnd2
*/
void follWnd(HWND hWnd1, HWND hWnd2)
{
	if (FOLL_WND) {
		return;
	}

	LONG left = 0;
	LONG top = 0;
	LONG right = 0;
	LONG bottom = 0;
	BOOL iconic = IsIconic(hWnd1);
	BOOL visiable = IsWindowVisible(hWnd1);
	LONG topmost = GetWindowLong(hWnd1, GWL_EXSTYLE) & WS_EX_TOPMOST;

	HWND lastActiveWnd = GetForegroundWindow();

	RECT r1{ 0 };

	RECT r2{ 0 };

	// ���ڸ���
	FOLL_WND = true;
	while (FOLL_WND) {
		BOOL tmpIconic = IsIconic(hWnd1);

		if (iconic != tmpIconic) {
			ShowWindow(hWnd2, tmpIconic ? SW_MINIMIZE : SW_RESTORE);
			iconic = tmpIconic;
		}

		GetWindowRect(hWnd1, &r1);

		LONG tmpTopmost = GetWindowLong(hWnd1, GWL_EXSTYLE) & WS_EX_TOPMOST;
		BOOL tmpVisiable = IsWindowVisible(hWnd1);
		HWND tmpActiveWnd = GetForegroundWindow();

		if (tmpActiveWnd != lastActiveWnd && tmpActiveWnd != hWnd1) {
			lastActiveWnd = tmpActiveWnd;
		}

		if (
			(tmpActiveWnd != lastActiveWnd && tmpActiveWnd == hWnd1) ||
			tmpVisiable != visiable || tmpTopmost != topmost
			|| left != r1.left || top != r1.top || right != r1.right || bottom != r1.bottom
			) {

			left = r1.left;
			top = r1.top;
			right = r1.right;
			bottom = r1.bottom;
			visiable = tmpVisiable;
			topmost = tmpTopmost;
			lastActiveWnd = tmpActiveWnd;

			//std::cout << hWnd1 << " hWnd1 >>>>>" << visiable << "..." << topmost << "..." << top << "..." << right << std::endl;

			GetWindowRect(hWnd2, &r2);

			SetWindowPos(hWnd2, topmost ? HWND_TOPMOST : hWnd1, right, top, r2.right - r2.left, bottom - top, visiable ? SWP_SHOWWINDOW | SWP_NOACTIVATE : SWP_HIDEWINDOW | SWP_NOACTIVATE);
		}

		Sleep(5);
	}

	FOLL_WND = false;
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

	SLOG_INFO(u8"WxAttacher��ʼ�����");

	// ��Ϣѭ��
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
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
 * @brief ����WM_COPYDATA��Ϣ
 * @param wmEvent
 * @param size
 * @param payload
*/
void sendWmMsg(DWORD wmEvent, DWORD size, PVOID payload)
{
	// ���͵�collector
	HWND hWnd = findWxCollectorWindow();
	if (hWnd == NULL)
	{
		SLOG_ERROR(u8"WM��Ϣ����ʧ��[δ�ҵ�WX_COLLECTOR], wmEvent:[{}], paylaod:[{}]", wmEvent, payload);
		return;
	}

	COPYDATASTRUCT data;
	data.dwData = wmEvent;
	data.cbData = size;
	data.lpData = payload;

	SendMessage(hWnd, WM_COPYDATA, (DWORD)GetModuleHandle(NULL), (LPARAM)&data);
}

/**
 * @brief discard����ws
*/
void discardLocalWs() {
	if (localWs) {
		localWs->discard();
	}
}

/**
 * @brief �رձ���ws
*/
void closeLocalWs() {
	if (localWs) {
		localWs->close();
	}
}

/**
 * @brief ���ͱ���ws��Ϣ
 * @param bodyType
 * @param body
*/
void sendToLocalWs(std::string bodyType, JsonObject body, bool log) {
	if (!localWs) {
		SLOG_ERROR(L"LocalWs - δ����");
		return;
	}

	JsonObject jsonObject;
	jsonObject.set("deliveryId", Strs::ws2s(UUIDs::generate()));
	jsonObject.set("deliveryTime", Times::currentLocalMillis());
	jsonObject.set("bodyType", bodyType);
	jsonObject.set("body", body);

	std::stringstream jsonStrStream;
	jsonObject.stringify(jsonStrStream, 0, 0);
	std::string jsonStr = jsonStrStream.str();

	if (log) {
		SLOG_INFO(u8"sendToLocalWs:[{}]", jsonStr);
	}
	localWs->sendMsg(jsonStr);
}

/**
 * @brief discardԶ��ws
*/
void discardRemoteWs() {
	if (remoteWs) {
		remoteWs->discard();
	}
}

/**
 * @brief �ر�Զ��ws
*/
void closeRemoteWs() {
	if (remoteWs) {
		remoteWs->close();
	}
}

/**
 * @brief ����Զ��ws��Ϣ
 * @param bodyType
 * @param body
*/
void sendToRemoteWs(
	std::wstring fromId, std::wstring fromType,
	std::wstring toId, std::wstring toType,
	std::wstring bodyType, JsonObject body,
	bool log
) {
	if (!remoteWs) {
		SLOG_ERROR(L"RemoteWs - δ����");
		return;
	}

	if (!wcslen(clientLoginInfo.userId)) {
		SLOG_ERROR(L"RemoteWs - δ��¼");
		return;
	}

	LONG64 deliveryTime = Times::currentSntpMillis();
	if (!deliveryTime) {
		SLOG_WARN(L"RemoteWs - SNTPʱ���쳣����ʹ�ñ���ʱ��");
		deliveryTime = Times::currentLocalMillis();
	}

	JsonObject jsonObject;
	jsonObject.set("deliveryId", Strs::ws2s(UUIDs::generate()));
	jsonObject.set("deliveryTime", deliveryTime);

	jsonObject.set("fromId", Strs::ws2s(fromId));
	jsonObject.set("fromType", Strs::ws2s(fromType));

	jsonObject.set("toId", Strs::ws2s(toId));
	jsonObject.set("toType", Strs::ws2s(toType));

	jsonObject.set("channelType", u8"WECHAT");

	jsonObject.set("bodyType", Strs::ws2s(bodyType));
	jsonObject.set("body", body);

	std::stringstream jsonStrStream;
	jsonObject.stringify(jsonStrStream, 0, 0);
	std::string jsonStr = jsonStrStream.str();

	if (log) {
		SLOG_INFO(u8"sendToRemoteWs:[{}]", jsonStr);
	}
	remoteWs->sendMsg(jsonStr);
}

/**
 * @brief Զ��ws SNTPʱ��ͬ��
 * @param sntpId ͬ��Ϣid
 * @param fromId
 * @param fromType
*/
void sntpRemoteWs(
	std::wstring sntpId,
	std::wstring fromId, std::wstring fromType
) {
	if (!remoteWs) {
		SLOG_ERROR(L"RemoteWs - δ����");
		return;
	}

	JsonObject jsonObject;
	jsonObject.set("deliveryId", Strs::ws2s(sntpId));
	jsonObject.set("deliveryTime", Times::currentLocalMillis());

	jsonObject.set("fromId", Strs::ws2s(fromId));
	jsonObject.set("fromType", Strs::ws2s(fromType));

	jsonObject.set("toId", u8"SNTP_SVC");
	jsonObject.set("toType", u8"SYSTEM");

	jsonObject.set("channelType", u8"WECHAT");

	jsonObject.set("bodyType", u8"CUSTOM");

	std::stringstream jsonStrStream;
	jsonObject.stringify(jsonStrStream, 0, 0);
	std::string jsonStr = jsonStrStream.str();

	SLOG_INFO(u8"sntpRemoteWs:[{}]", jsonStr);

	Times::restartSntpTicker();
	remoteWs->sendMsg(jsonStr);
}

/**
 * @brief ����ws opened callback
*/
void localWsOpenedCallback() {

}

/**
 * @brief Զ��ws opened callback
*/
void remoteWsOpenedCallback() {

}

/**
 * @brief ��ʼ��
 * @param hModule
*/
void init(HMODULE hModule)
{
	// ����ws session
	if (!(ENV_CONF_LOCAL_WS_HOST.empty() || !ENV_CONF_LOCAL_WS_PORT || ENV_CONF_LOCAL_WS_PATH.empty())) {
		localWs = new WsSession(Strs::ws2s(ENV_CONF_LOCAL_WS_HOST), ENV_CONF_LOCAL_WS_PORT, Strs::ws2s(ENV_CONF_LOCAL_WS_PATH), "", ENV_CONF_LOCAL_WS_SSL, Strs::ws2s(ENV_CONF_LOCAL_WS_ENCRYPT), localWsOpenedCallback, receiveFromLocalWs);
	}

	// Զ��ws session
	if (!(ENV_CONF_REMOTE_WS_HOST.empty() || !ENV_CONF_REMOTE_WS_PORT || ENV_CONF_REMOTE_WS_PATH.empty())) {
		remoteWs = new WsSession(Strs::ws2s(ENV_CONF_REMOTE_WS_HOST), ENV_CONF_REMOTE_WS_PORT, Strs::ws2s(ENV_CONF_REMOTE_WS_PATH), Base64::encode(strrev((LPSTR)Strs::ws2s(clientLoginInfo.token).data())), ENV_CONF_REMOTE_WS_SSL, Strs::ws2s(ENV_CONF_REMOTE_WS_ENCRYPT), remoteWsOpenedCallback, receiveFromRemoteWs);
	}

	// ע�ᴰ��
	registerWindow(hModule, L"WX_ATTACHER", L"WX_ATTACHER", wndProc);

}
