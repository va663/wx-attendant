#include "pch.h"
#include <Shlwapi.h>
#include <future>
#include <thread>
#pragma comment(lib,"Shlwapi.lib")

/**
 * @brief �����յ���΢����Ϣ
 * @param msg
*/
void handleReceiveMsg(WxReceiveMsg* msg) {
	try {
		switch (msg->msgType) {
		case 0x01:
		{
			memcpy(msg->msgTypeDesc, L"�ı�", sizeof(L"�ı�"));
		}
		break;
		default:
		{
			// ����
			delete msg;
			return;
		}
		break;
		}

		WxContactInfo sender = { 0 };
		if (
			!Strs::len(msg->senderWxid)
			|| !getContactInfo(msg->senderWxid, sender)
			) {
			SLOG_ERROR(L"΢����Ϣ���մ����쳣���޷���ȡ���ͷ���Ϣ - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, content:{}]",
				msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->content);
			delete msg;
			return;
		}

		if (!(sender.type == 1 || Strs::len(msg->groupWxid))) {
			// �Ȳ��ǵ���Ҳ����Ⱥ�������
			delete msg;
			return;
		}

		swprintf_s(msg->senderWxcid, sender.wxcid);
		swprintf_s(msg->senderWxname, sender.wxname);
		swprintf_s(msg->senderWxremark, sender.wxremark);
		swprintf_s(msg->senderAvatar, sender.avatar);
		swprintf_s(msg->senderDisplayName, Strs::len(sender.wxremark) ? sender.wxremark : sender.wxname);

		// �Ƿ������Ϣ
		msg->inFriends = TRUE;

		// Ⱥ��
		if (Strs::len(msg->groupWxid)) {
			// �Ƿ������Ϣ
			WxContactInfo tmp = { 0 };
			msg->inFriends = getFriendContactInfo(msg->senderWxid, tmp);

			// Ⱥ��Ϣ
			WxContactInfo groupContactInfo = { 0 };
			getContactInfo(msg->groupWxid, groupContactInfo);
			swprintf_s(msg->groupWxname, groupContactInfo.wxname);
			swprintf_s(msg->groupWxremark, groupContactInfo.wxremark);
			swprintf_s(msg->groupAvatar, groupContactInfo.avatar);

			// Ⱥ�����ȡȺ�ǳ� TODO
		}


		// ���շ�Ϊ��ǰ�û�
		WxLoginInfo loginInfo = getLoginInfo();
		swprintf_s(msg->recipientWxid, loginInfo.wxid);
		swprintf_s(msg->recipientWxcid, loginInfo.wxcid);
		swprintf_s(msg->recipientWxname, loginInfo.wxname);
		swprintf_s(msg->recipientAvatar, loginInfo.avatar);

		SLOG_INFO(L"�յ�΢����Ϣ - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderWxcid:{}, senderWxname:{}, senderWxremark:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, atWxid:{}, content:{}]",
			msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderWxcid, msg->senderWxname, msg->senderWxremark, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->atWxid, msg->content);

		// �ϱ�
		sendWmMsg(WM_COPYDATA_EVENT_WX_MSG_CAPTURE, sizeof(WxReceiveMsg), (PVOID)msg);

		delete msg;
	}
	catch (...) {
		SLOG_ERROR(L"΢����Ϣ���մ����쳣 - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, content:{}]",
			msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->content);
		delete msg;
	}

}

/**
 * @brief ������Ϣ����
 * @param rEsi esi �Ĵ���
 * @return
*/
void __stdcall receiveMsg(DWORD rEsi) {
	try
	{
		if (IsBadReadPtr((void*)rEsi, 4) ||
			IsBadReadPtr((void*)(rEsi + WX_OFFSET_RECEIVE_MSG_WXID), 4) ||
			IsBadReadPtr((void*)(rEsi + WX_OFFSET_RECEIVE_MSG_SECOND_WXID), 4) ||
			IsBadReadPtr((void*)(rEsi + WX_OFFSET_RECEIVE_MSG_TYPE), 4) ||
			IsBadReadPtr((void*)(rEsi + WX_OFFSET_RECEIVE_MSG_OWN), 4) ||
			IsBadReadPtr((void*)(rEsi + WX_OFFSET_RECEIVE_MSG_CONTENT), 4))
		{
			SLOG_ERROR(L"΢����Ϣ���� - [�ڴ��޷���ȡ]");
			return;
		}

		// ��Ϣ���ͬ��
		if (*(LPDWORD)(rEsi + WX_OFFSET_RECEIVE_MSG_OWN)) {
			return;
		}

		DWORD msgType = *(LPDWORD)(rEsi + WX_OFFSET_RECEIVE_MSG_TYPE);
		LPWSTR wxid = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_WXID));
		LPWSTR secondWxid = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_SECOND_WXID));
		LPWSTR content = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_CONTENT));

		if (msgType != 0x01) {
			// ���Բ���Ҫ����Ϣ����
			return;
		}
		// ��exclude����ϵ����Ϣ
		if (isExcludedContactInfo(wxid)) {
			return;
		}

		WxReceiveMsg* msg = new WxReceiveMsg{ 0 };
		memset(msg, 0, sizeof(WxReceiveMsg));

		msg->msgType = msgType;

		if (!secondWxid) {
			swprintf_s(msg->senderWxid, wxid);
		}
		else {
			swprintf_s(msg->senderWxid, secondWxid);
			swprintf_s(msg->groupWxid, wxid);
		}

		swprintf_s(msg->content, content);

		// �첽������Ϣ
		std::thread(handleReceiveMsg, msg).detach();
	}
	catch (...) {
		SLOG_ERROR(L"΢����Ϣ�����쳣");
	}

}

DWORD RECEIVE_MSG_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_CALL;
DWORD RECEIVE_MSG_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_CALL_RET;

/**
 * @brief ������Ϣ
*/
__declspec(naked) void receiveMsgHook()
{
	__asm
	{
		pushad
		pushfd

		push esi
		call receiveMsg

		popfd
		popad

		// ����ԭ����call
		call RECEIVE_MSG_CALL_ADDR
		// ��ת��ԭ��ַ
		jmp RECEIVE_MSG_CALL_RET_ADDR
	}
}

/**
 * @brief ����
*/
void testHandleReceiveMsg() {
	while (!ENV_MODULE_DETACHED) {
		WxReceiveMsg* msg = new WxReceiveMsg{ 0 };
		memset(msg, 0, sizeof(WxReceiveMsg));

		msg->msgType = Times::currentLocalMillis() % 2 + 1;
		swprintf_s(msg->senderWxid, L"wxid_xxx2");
		swprintf_s(msg->content, L"���Ǹ¶���Ϊ��Ϊ�ҹ����˹��ҹ���΢���������󰡵ĸ�λ�����gasaegtaewagasgewateraasdgwatawbasrwafasdrewafsdfasd");

		std::thread(handleReceiveMsg, msg).detach();
		Sleep(500);
	}
}

/**
 * @brief hook������Ϣ
*/
void hookReceiveMsg() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_HOOK, receiveMsgHook);

	// ����
	//std::thread(testHandleReceiveMsg).detach();

}