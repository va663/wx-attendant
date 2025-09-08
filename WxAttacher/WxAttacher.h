#pragma once

#include <windows.h>
#include <map>

#include "utils.h"
#include "procevent.h"

#define WX_PROCESS_NAME "WeChat.exe"
#define WX_WECHATWINDLL "WeChatWin.dll"
#define WX_ATTACHER_PROCESS_NAME "WxAttacher.exe"
#define WX_COLLECTOR_DLL_NAME "WxCollector.dll"

/**
 * @brief �ͻ��˵�¼�û���Ϣ
*/
struct ClientLoginInfo {
	WCHAR tenantId[0x40];
	WCHAR userId[0x40];
	WCHAR token[0x100];
};

/**
 * @brief ΢�ŵ�¼�û���Ϣ
*/
struct WxLoginInfo {
	WCHAR wxid[0x40];
	WCHAR wxcid[0x40];
	WCHAR phone[0x40];
	WCHAR wxname[0x80];
	WCHAR avatar[0x100];
};

/**
 * @brief ��ϵ����Ϣ
*/
struct WxContactInfo {
	DWORD type;           // 1 / �����û� 3 / Ⱥ��
	WCHAR wxid[0x40];
	WCHAR wxcid[0x40];
	WCHAR wxname[0x80];
	WCHAR wxremark[0x80];
	WCHAR avatar[0x100];
};

/**
 * @brief ���յ�����Ϣ�ṹ��
*/
struct WxReceiveMsg
{
	DWORD msgType;				        //��Ϣ����
	WCHAR msgTypeDesc[0x10];			//��Ϣ��������
	WCHAR senderWxid[0x40];
	WCHAR senderWxcid[0x40];
	WCHAR senderWxname[0x80];
	WCHAR senderWxremark[0x80];
	WCHAR senderAvatar[0x100];
	WCHAR senderDisplayName[0x80];
	BOOL inFriends;                     //�Ƿ������Ϣ
	WCHAR recipientWxid[0x40];
	WCHAR recipientWxcid[0x40];
	WCHAR recipientWxname[0x80];
	WCHAR recipientAvatar[0x100];
	WCHAR groupWxid[0x40];
	WCHAR groupWxname[0x80];
	WCHAR groupWxremark[0x80];
	WCHAR groupAvatar[0x100];
	WCHAR atWxid[0x40];
	WCHAR content[0x4000];	            //��Ϣ����
};

/**
 * @brief �����ı���Ϣ
*/
struct WxSendTextMsg
{
	WCHAR fromWxid[0x40];			//���ͷ�΢��ID
	WCHAR toWxid[0x40];			    //���շ�΢��ID
	WCHAR atWxid[0x40];			    //at��΢��ID
	WCHAR content[0x4000];	        //��Ϣ����
};


void initEnv();
void loadConf();

void init(HMODULE hModule);

void openConsole();
void freeConsole();

HWND findWxAttacherWindow();
HWND findWxAttacherDebugConsoleWindow();
HWND findWxCollectorWindow();

void follWnd(HWND hWnd1, HWND hWnd2);

std::vector<DWORD> findPID(LPCWSTR processName);
DWORD findWxPID(std::wstring wxid);

std::map<DWORD, WxLoginInfo> findRunningWxLoginInfo();

HMODULE getModule(DWORD pid, LPCWSTR mName);
std::string getVersion(HMODULE hModule);

int attach(DWORD pid, LPCWSTR dllPath);
int detach(DWORD pid, LPCWSTR dllPath);

int attachWxCollector(std::wstring wxid);
int detachWxCollector();

void receiveWmMsg(COPYDATASTRUCT* pCopyData);
void sendWmMsg(DWORD wmEvent, DWORD size, PVOID payload);

void receiveFromRemoteWs(std::string wsMsg);
void receiveFromLocalWs(std::string wsMsg);

/////////////////////////////////////////////
// ȫ�ֱ�������

extern std::map<std::wstring, std::wstring> startupOptions;

extern std::wstring ENV_EXE_PATH;
extern std::wstring ENV_EXE_ABSOLUTE_DIR;

extern std::wstring ENV_WX_COLLECTOR_DLL_PATH;
extern std::wstring ENV_WX_COLLECTOR_ABSOLUTE_DIR;

extern std::wstring ENV_CONF_LOCAL_WS_HOST;
extern int ENV_CONF_LOCAL_WS_PORT;
extern std::wstring ENV_CONF_LOCAL_WS_PATH;

extern std::wstring ENV_CONF_REMOTE_WS_HOST;
extern int ENV_CONF_REMOTE_WS_PORT;
extern std::wstring ENV_CONF_REMOTE_WS_PATH;

extern ClientLoginInfo clientLoginInfo;
extern WxLoginInfo wxLoginInfo;
////////////////////////////////////////////