#include "pch.h"
#include <thread>

/**
 * @brief 处理聊天窗口切换
*/
void handleChatContactSwitch(LPWSTR wxid) {
	try {
		WxContactInfo contactInfo = { 0 };
		// 对于被exclude的联系人或自身则返回空
		if (
			!(isExcludedContactInfo(wxid) || Strs::equalsIgnoreCase(getLoginInfo().wxid, wxid))
			) {
			if (!getContactInfo(wxid, contactInfo)) {
				SLOG_ERROR(L"微信聊天窗口切换 - 获取到无效的wxid[wxid:{}]", wxid);
				return;
			}
			// 非个人用户则忽略，clear信息
			if (contactInfo.type != 1) {
				contactInfo = { 0 };
			}
		}

		SLOG_DEBUG(L"微信聊天窗口切换 - [wxid:{}, wxcid:{}, wxname:{}, wxremark:{}, avatar:{}]",
			contactInfo.wxid, contactInfo.wxcid, contactInfo.wxname, contactInfo.wxremark, contactInfo.avatar);

		// 上报
		sendWmMsg(WM_COPYDATA_EVENT_WX_CHAT_CONTACT_SWITCH, sizeof(WxContactInfo), (PVOID)&contactInfo);
	}
	catch (...) {
		SLOG_ERROR(L"微信聊天窗口切换处理异常 - [wxid:{}]", wxid);
	}

}

/**
 * @brief 聊天窗口切换
 * @param rEax eax 寄存器
 * @return
*/
void __stdcall chatContactSwitch(DWORD rEax) {
	try
	{
		if (IsBadReadPtr(*(LPWSTR*)rEax, 4)) {
			SLOG_ERROR(L"微信聊天窗口切换 - [内存无法读取 addr:{} *addr:{}]", rEax, (DWORD)(*(LPWSTR*)rEax));
			return;
		}
		LPWSTR curChatContactWxid = *(LPWSTR*)rEax;

		// 异步处理
		std::thread(handleChatContactSwitch, curChatContactWxid).detach();
	}
	catch (...) {
		SLOG_ERROR(L"微信聊天窗口切换异常");
	}

}

DWORD CHAT_CONTACT_SWITCH_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_CALL;
DWORD CHAT_CONTACT_SWITCH_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_CALL_RET;

/**
 * @brief 聊天窗口切换
*/
__declspec(naked) void chatContactSwitchHook()
{
	__asm
	{
		pushad
		pushfd

		push eax
		call chatContactSwitch

		popfd
		popad

		// 调用原来的call
		call CHAT_CONTACT_SWITCH_CALL_ADDR
		// 跳转回原地址
		jmp CHAT_CONTACT_SWITCH_CALL_RET_ADDR
	}
}


/**
 * @brief hook聊天窗口切换
*/
void hookChatContactSwitch() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_HOOK, chatContactSwitchHook);
}