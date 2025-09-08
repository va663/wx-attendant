#include "pch.h"

/**
 * @brief 发送文本消息
 * @param wxid
 * @param content
 * @param atWxid
*/
void sendTextMsg(LPWSTR wxid, LPWSTR content, LPWSTR atWxid) {
	SLOG_INFO(L"发送微信文本消息 - [toWxid:{}, atWxid:{}, content:{}]", wxid, atWxid, content);

	DWORD dwSendCallAddr = getWeChatWinDllBase() + WX_OFFSET_SEND_TEXT_MSG_CALL;

	// 发送目标构建
	WxStr to = { 0 };
	memset(&to, 0, sizeof(WxStr));
	to.pStr = wxid;
	to.strLen = Strs::len(wxid);
	to.strLen2 = to.strLen;

	WxStr* toAddr = &to;

	// @目标构建
	WxAt at = { 0 };
	memset(&at, 0, sizeof(WxAt));
	if (atWxid && Strs::len(atWxid)) {
		// @XXX
		WxContactInfo atSomeone = { 0 };
		if (getContactInfo(atWxid, atSomeone)) {
			std::wstring tmpContent;
			tmpContent.append(L"@").append(atSomeone.wxname).append(L"\u2005")
				.append(L"\n")
				.append(content);
			content = (LPWSTR)tmpContent.data();

			WxStr atid = { 0 };
			memset(&atid, 0, sizeof(WxStr));
			atid.pStr = atWxid;
			atid.strLen = Strs::len(atWxid);
			atid.strLen2 = atid.strLen;

			at.pWxid = &atid;

			// 指向atid之后
			at.p1 = (DWORD)(at.pWxid + 1);
			at.p2 = at.p1;
		}
	}

	// 消息体构建
	WxStr msg = { 0 };
	memset(&msg, 0, sizeof(WxStr));
	msg.pStr = content;
	msg.strLen = Strs::len(content);
	msg.strLen2 = msg.strLen;

	WxStr* msgAddr = &msg;

	char buf[0x400];

	__asm {
		pushad
		pushfd

		push 0x1
		lea eax, at
		push eax
		mov edi, msgAddr
		push edi

		mov edx, toAddr

		// ecx指向this指针，暂时没找到。。。
		lea ecx, buf

		call dwSendCallAddr

		add esp, 0xC

		popfd
		popad
	}

}