#pragma once

#define WX_VERSION "3.6.0.18"
#define WX_WECHATWINDLL "WeChatWin.dll"

#define WX_OFFSET_LOGIN_FLAG 0x222EC90
#define WX_OFFSET_LOGIN_WXID 0x222EB3C
#define WX_OFFSET_LOGIN_WXCID 0x222ED30
#define WX_OFFSET_LOGIN_PHONE 0x222EBE8
#define WX_OFFSET_LOGIN_WXNAME 0x222EBB4
#define WX_OFFSET_LOGIN_AVATAR 0x222EE94

#define WX_OFFSET_LOGOUT_HOOK 0x466C43
#define WX_OFFSET_LOGOUT_CALL_RET 0x466C48
#define WX_OFFSET_LOGOUT_CALL 0x7005D0

#define WX_OFFSET_CONTACT_LIST 0x222F3BC
#define WX_OFFSET_CONTACT_LIST_NODE_ADDR 0x4C

#define WX_OFFSET_CHAT_CONTACT_SWITCH_HOOK 0x50C417
#define WX_OFFSET_CHAT_CONTACT_SWITCH_CALL_RET 0x50C41C
#define WX_OFFSET_CHAT_CONTACT_SWITCH_CALL 0x701DC0

#define WX_OFFSET_SEND_TEXT_MSG_CALL 0x4BE7B0

#define WX_OFFSET_RECEIVE_MSG_HOOK 0x3C6651
#define WX_OFFSET_RECEIVE_MSG_CALL_RET 0x3C6656
#define WX_OFFSET_RECEIVE_MSG_CALL 0x22C0C0

#define WX_OFFSET_RECEIVE_MSG_WXID 0x48
#define WX_OFFSET_RECEIVE_MSG_SECOND_WXID 0x170
#define WX_OFFSET_RECEIVE_MSG_TYPE 0x38
#define WX_OFFSET_RECEIVE_MSG_OWN 0x3C
#define WX_OFFSET_RECEIVE_MSG_CONTENT 0x70

#define WX_OFFSET_GROUP_BUF_BUILD_CALL 0x64BBB0
#define WX_OFFSET_GROUP_INFO_GET_CALL 0x3E46D0


/////////////////////////////////////////////
// 全局变量声明

extern std::wstring ENV_EXE_PATH;
extern std::wstring ENV_EXE_ABSOLUTE_DIR;

extern std::wstring ENV_MOUDLE_PATH;
extern std::wstring ENV_MOUDLE_ABSOLUTE_DIR;
extern std::wstring ENV_MOUDLE_FILENAME;
extern BOOL ENV_MODULE_DETACHED;

////////////////////////////////////////////

/**
 * @brief wx str 结构体
*/
struct WxStr {
	PWCHAR pStr;
	int strLen;
	int strLen2;
	char buff[0x8];
};

/**
 * @brief wx at结构体
*/
struct WxAt {
	WxStr* pWxid;
	DWORD p1;
	DWORD p2;
	int end = 0;
};

/**
 * @brief 微信登录用户信息
*/
struct WxLoginInfo {
	WCHAR wxid[0x40];
	WCHAR wxcid[0x40];
	WCHAR phone[0x40];
	WCHAR wxname[0x80];
	WCHAR avatar[0x100];
};

/**
 * @brief 联系人信息
*/
struct WxContactInfo {
	DWORD type;           // 1 / 个人用户 3 / 群聊
	WCHAR wxid[0x40];
	WCHAR wxcid[0x40];
	WCHAR wxname[0x80];
	WCHAR wxremark[0x80];
	WCHAR avatar[0x100];
};

/**
 * @brief 群信息
*/
struct WxGroupInfo {
	WCHAR wxid[0x40];
	WCHAR adminWxid[0x40];
	WCHAR wxname[0x80];
	WCHAR wxremark[0x80];
	WCHAR avatar[0x100];
	std::vector<std::wstring> members;
};

/**
 * @brief 接收到的消息结构体
*/
struct WxReceiveMsg
{
	DWORD msgType;				        //消息类型
	WCHAR msgTypeDesc[0x10];			//消息类型描述
	WCHAR senderWxid[0x40];
	WCHAR senderWxcid[0x40];
	WCHAR senderWxname[0x80];
	WCHAR senderWxremark[0x80];
	WCHAR senderAvatar[0x100];
	WCHAR senderDisplayName[0x80];
	BOOL inFriends;                     //是否好友消息
	WCHAR recipientWxid[0x40];
	WCHAR recipientWxcid[0x40];
	WCHAR recipientWxname[0x80];
	WCHAR recipientAvatar[0x100];
	WCHAR groupWxid[0x40];
	WCHAR groupWxname[0x80];
	WCHAR groupWxremark[0x80];
	WCHAR groupAvatar[0x100];
	WCHAR atWxid[0x40];
	WCHAR content[0x4000];	            //消息内容
};

/**
 * @brief 发送文本消息
*/
struct WxSendTextMsg
{
	WCHAR fromWxid[0x40];			//发送方微信ID
	WCHAR toWxid[0x40];			    //接收方微信ID
	WCHAR atWxid[0x40];			    //at方微信ID
	WCHAR content[0x4000];	        //消息内容
};

void initModuleEnv(HMODULE hModule);
void init(HMODULE hModule);

std::string getVersion(HMODULE hModule);

void openConsole();
void freeConsole();
void writeConsole(std::wstring str);

HWND findWxAttacherWindow();

void hookAnyAddress(DWORD dwHookAddr, LPVOID dwJmpAddress);
void hookRecovery(DWORD dwHookAddr);
void hookRecoveryAll();

DWORD getWeChatWinDllBase();

bool loggedin();
WxLoginInfo getLoginInfo();

void buildFriendContactInfoMap();
std::vector<WxContactInfo> getFriendContactInfoAll();
BOOL getFriendContactInfo(std::wstring wxid, WxContactInfo& info);
BOOL getContactInfo(std::wstring wxid, WxContactInfo& info);
BOOL getGroupInfo(std::wstring wxid, WxGroupInfo& info);
BOOL isExcludedContactInfo(std::wstring wxid);

void hookLogout();
void hookChatContactSwitch();

void sendTextMsg(LPWSTR wxid, LPWSTR content, LPWSTR atWxid);
void hookReceiveMsg();

void receiveWmMsg(COPYDATASTRUCT* pCopyData);
void sendWmMsg(DWORD wmEvent, DWORD size, PVOID payload);
void sendWmNoticeMsg(LPCWSTR msg);