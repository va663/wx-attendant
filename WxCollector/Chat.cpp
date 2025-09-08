#include "pch.h"
#include <thread>

/**
 * @brief �������촰���л�
*/
void handleChatContactSwitch(LPWSTR wxid) {
	try {
		WxContactInfo contactInfo = { 0 };
		// ���ڱ�exclude����ϵ�˻������򷵻ؿ�
		if (
			!(isExcludedContactInfo(wxid) || Strs::equalsIgnoreCase(getLoginInfo().wxid, wxid))
			) {
			if (!getContactInfo(wxid, contactInfo)) {
				SLOG_ERROR(L"΢�����촰���л� - ��ȡ����Ч��wxid[wxid:{}]", wxid);
				return;
			}
			// �Ǹ����û�����ԣ�clear��Ϣ
			if (contactInfo.type != 1) {
				contactInfo = { 0 };
			}
		}

		SLOG_DEBUG(L"΢�����촰���л� - [wxid:{}, wxcid:{}, wxname:{}, wxremark:{}, avatar:{}]",
			contactInfo.wxid, contactInfo.wxcid, contactInfo.wxname, contactInfo.wxremark, contactInfo.avatar);

		// �ϱ�
		sendWmMsg(WM_COPYDATA_EVENT_WX_CHAT_CONTACT_SWITCH, sizeof(WxContactInfo), (PVOID)&contactInfo);
	}
	catch (...) {
		SLOG_ERROR(L"΢�����촰���л������쳣 - [wxid:{}]", wxid);
	}

}

/**
 * @brief ���촰���л�
 * @param rEax eax �Ĵ���
 * @return
*/
void __stdcall chatContactSwitch(DWORD rEax) {
	try
	{
		if (IsBadReadPtr(*(LPWSTR*)rEax, 4)) {
			SLOG_ERROR(L"΢�����촰���л� - [�ڴ��޷���ȡ addr:{} *addr:{}]", rEax, (DWORD)(*(LPWSTR*)rEax));
			return;
		}
		LPWSTR curChatContactWxid = *(LPWSTR*)rEax;

		// �첽����
		std::thread(handleChatContactSwitch, curChatContactWxid).detach();
	}
	catch (...) {
		SLOG_ERROR(L"΢�����촰���л��쳣");
	}

}

DWORD CHAT_CONTACT_SWITCH_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_CALL;
DWORD CHAT_CONTACT_SWITCH_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_CALL_RET;

/**
 * @brief ���촰���л�
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

		// ����ԭ����call
		call CHAT_CONTACT_SWITCH_CALL_ADDR
		// ��ת��ԭ��ַ
		jmp CHAT_CONTACT_SWITCH_CALL_RET_ADDR
	}
}


/**
 * @brief hook���촰���л�
*/
void hookChatContactSwitch() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_CHAT_CONTACT_SWITCH_HOOK, chatContactSwitchHook);
}