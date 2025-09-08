#include "pch.h"
#include <regex>

/**
 * @brief ��¼���
 * @return
*/
bool loggedin() {
	return *((PDWORD)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_FLAG)) != 0;
}

/**
 * @brief �Ƿ���Ч��wxid
 * @param pWxid
 * @return
*/
bool validWxid(PCHAR pWxid) {
	std::regex wxidRegex("[a-zA-Z0-9-_]{6,20}");
	return std::regex_match(pWxid, wxidRegex);
}

/**
 * @brief ��ȡ��¼�û���Ϣ
 * @return
*/
WxLoginInfo getLoginInfo() {
	WxLoginInfo loginInfo{ 0 };

	// �õ�ַ���ܴ�ָ���������ֵ���������δ֪
	PCHAR pLoginWxid = *(PCHAR*)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_WXID);
	if (!IsBadReadPtr(pLoginWxid, 4) && validWxid(pLoginWxid)) {
		swprintf_s(loginInfo.wxid, Strs::s2ws(pLoginWxid).data());
	}
	else {
		pLoginWxid = (PCHAR)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_WXID);
		if (!IsBadReadPtr(pLoginWxid, 4) && validWxid(pLoginWxid)) {
			swprintf_s(loginInfo.wxid, Strs::s2ws(pLoginWxid).data());
		}
	}

	PCHAR pLoginWxcid = (PCHAR)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_WXCID);
	if (!IsBadReadPtr(pLoginWxcid, 4)) {
		swprintf_s(loginInfo.wxcid, Strs::s2ws(pLoginWxcid).data());
	}

	PCHAR pLoginPhone = (PCHAR)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_PHONE);
	if (!IsBadReadPtr(pLoginPhone, 4)) {
		swprintf_s(loginInfo.phone, Strs::s2ws(pLoginPhone).data());
	}

	PCHAR pLoginWxname = (PCHAR)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_WXNAME);
	if (!IsBadReadPtr(pLoginWxname, 4)) {
		swprintf_s(loginInfo.wxname, Strs::s2ws(pLoginWxname).data());
	}

	PCHAR pLoginAvatar = *(PCHAR*)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_AVATAR);
	if (!IsBadReadPtr(pLoginAvatar, 4)) {
		swprintf_s(loginInfo.avatar, Strs::s2ws(pLoginAvatar).data());
	}

	return loginInfo;
}

/**
 * @brief �����˳���¼
*/
void handleLogout() {
	try {
		WxLoginInfo loginInfo = getLoginInfo();
		SLOG_INFO(L"΢���˳���¼ - [wxid:{}, wxname:{}]", loginInfo.wxid, loginInfo.wxname);
		sendWmMsg(WM_COPYDATA_EVENT_WX_USER_LOGOUT, 0, NULL);
	}
	catch (...) {
		SLOG_ERROR(L"΢���˳���¼�����쳣");
	}
}

/**
 * @brief �˳���¼
 * @return
*/
void __stdcall logout() {
	try
	{
		// �첽����
		std::thread(handleLogout).detach();
	}
	catch (...) {
		SLOG_ERROR(L"΢���˳���¼�����쳣");
	}
}

DWORD LOGOUT_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_LOGOUT_CALL;
DWORD LOGOUT_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_LOGOUT_CALL_RET;

/**
 * @brief �˳���¼
*/
__declspec(naked) void logoutHook()
{
	__asm
	{
		pushad
		pushfd

		call logout

		popfd
		popad

		// ����ԭ����call
		call LOGOUT_CALL_ADDR
		// ��ת��ԭ��ַ
		jmp LOGOUT_CALL_RET_ADDR
	}
}

/**
 * @brief hook�˳���¼
*/
void hookLogout() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_LOGOUT_HOOK, logoutHook);
}