#include "pch.h"
#include <regex>

/**
 * @brief 登录标记
 * @return
*/
bool loggedin() {
	return *((PDWORD)(getWeChatWinDllBase() + WX_OFFSET_LOGIN_FLAG)) != 0;
}

/**
 * @brief 是否有效的wxid
 * @param pWxid
 * @return
*/
bool validWxid(PCHAR pWxid) {
	std::regex wxidRegex("[a-zA-Z0-9-_]{6,20}");
	return std::regex_match(pWxid, wxidRegex);
}

/**
 * @brief 获取登录用户信息
 * @return
*/
WxLoginInfo getLoginInfo() {
	WxLoginInfo loginInfo{ 0 };

	// 该地址可能存指针或者字面值，具体机制未知
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
 * @brief 处理退出登录
*/
void handleLogout() {
	try {
		WxLoginInfo loginInfo = getLoginInfo();
		SLOG_INFO(L"微信退出登录 - [wxid:{}, wxname:{}]", loginInfo.wxid, loginInfo.wxname);
		sendWmMsg(WM_COPYDATA_EVENT_WX_USER_LOGOUT, 0, NULL);
	}
	catch (...) {
		SLOG_ERROR(L"微信退出登录处理异常");
	}
}

/**
 * @brief 退出登录
 * @return
*/
void __stdcall logout() {
	try
	{
		// 异步处理
		std::thread(handleLogout).detach();
	}
	catch (...) {
		SLOG_ERROR(L"微信退出登录处理异常");
	}
}

DWORD LOGOUT_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_LOGOUT_CALL;
DWORD LOGOUT_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_LOGOUT_CALL_RET;

/**
 * @brief 退出登录
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

		// 调用原来的call
		call LOGOUT_CALL_ADDR
		// 跳转回原地址
		jmp LOGOUT_CALL_RET_ADDR
	}
}

/**
 * @brief hook退出登录
*/
void hookLogout() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_LOGOUT_HOOK, logoutHook);
}