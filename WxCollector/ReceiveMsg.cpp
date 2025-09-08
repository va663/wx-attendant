#include "pch.h"
#include <Shlwapi.h>
#include <future>
#include <thread>
#pragma comment(lib,"Shlwapi.lib")

/**
 * @brief 处理收到的微信消息
 * @param msg
*/
void handleReceiveMsg(WxReceiveMsg* msg) {
	try {
		switch (msg->msgType) {
		case 0x01:
		{
			memcpy(msg->msgTypeDesc, L"文本", sizeof(L"文本"));
		}
		break;
		default:
		{
			// 忽略
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
			SLOG_ERROR(L"微信消息接收处理异常，无法获取发送方信息 - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, content:{}]",
				msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->content);
			delete msg;
			return;
		}

		if (!(sender.type == 1 || Strs::len(msg->groupWxid))) {
			// 既不是单聊也不是群聊则忽略
			delete msg;
			return;
		}

		swprintf_s(msg->senderWxcid, sender.wxcid);
		swprintf_s(msg->senderWxname, sender.wxname);
		swprintf_s(msg->senderWxremark, sender.wxremark);
		swprintf_s(msg->senderAvatar, sender.avatar);
		swprintf_s(msg->senderDisplayName, Strs::len(sender.wxremark) ? sender.wxremark : sender.wxname);

		// 是否好友消息
		msg->inFriends = TRUE;

		// 群聊
		if (Strs::len(msg->groupWxid)) {
			// 是否好友消息
			WxContactInfo tmp = { 0 };
			msg->inFriends = getFriendContactInfo(msg->senderWxid, tmp);

			// 群信息
			WxContactInfo groupContactInfo = { 0 };
			getContactInfo(msg->groupWxid, groupContactInfo);
			swprintf_s(msg->groupWxname, groupContactInfo.wxname);
			swprintf_s(msg->groupWxremark, groupContactInfo.wxremark);
			swprintf_s(msg->groupAvatar, groupContactInfo.avatar);

			// 群聊则获取群昵称 TODO
		}


		// 接收方为当前用户
		WxLoginInfo loginInfo = getLoginInfo();
		swprintf_s(msg->recipientWxid, loginInfo.wxid);
		swprintf_s(msg->recipientWxcid, loginInfo.wxcid);
		swprintf_s(msg->recipientWxname, loginInfo.wxname);
		swprintf_s(msg->recipientAvatar, loginInfo.avatar);

		SLOG_INFO(L"收到微信消息 - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderWxcid:{}, senderWxname:{}, senderWxremark:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, atWxid:{}, content:{}]",
			msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderWxcid, msg->senderWxname, msg->senderWxremark, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->atWxid, msg->content);

		// 上报
		sendWmMsg(WM_COPYDATA_EVENT_WX_MSG_CAPTURE, sizeof(WxReceiveMsg), (PVOID)msg);

		delete msg;
	}
	catch (...) {
		SLOG_ERROR(L"微信消息接收处理异常 - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, content:{}]",
			msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->content);
		delete msg;
	}

}

/**
 * @brief 接收消息函数
 * @param rEsi esi 寄存器
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
			SLOG_ERROR(L"微信消息接收 - [内存无法读取]");
			return;
		}

		// 消息多端同步
		if (*(LPDWORD)(rEsi + WX_OFFSET_RECEIVE_MSG_OWN)) {
			return;
		}

		DWORD msgType = *(LPDWORD)(rEsi + WX_OFFSET_RECEIVE_MSG_TYPE);
		LPWSTR wxid = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_WXID));
		LPWSTR secondWxid = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_SECOND_WXID));
		LPWSTR content = *((LPWSTR*)(rEsi + WX_OFFSET_RECEIVE_MSG_CONTENT));

		if (msgType != 0x01) {
			// 忽略不需要的消息类型
			return;
		}
		// 被exclude的联系人消息
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

		// 异步处理消息
		std::thread(handleReceiveMsg, msg).detach();
	}
	catch (...) {
		SLOG_ERROR(L"微信消息接收异常");
	}

}

DWORD RECEIVE_MSG_CALL_ADDR = getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_CALL;
DWORD RECEIVE_MSG_CALL_RET_ADDR = getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_CALL_RET;

/**
 * @brief 接收消息
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

		// 调用原来的call
		call RECEIVE_MSG_CALL_ADDR
		// 跳转回原地址
		jmp RECEIVE_MSG_CALL_RET_ADDR
	}
}

/**
 * @brief 测试
*/
void testHandleReceiveMsg() {
	while (!ENV_MODULE_DETACHED) {
		WxReceiveMsg* msg = new WxReceiveMsg{ 0 };
		memset(msg, 0, sizeof(WxReceiveMsg));

		msg->msgType = Times::currentLocalMillis() % 2 + 1;
		swprintf_s(msg->senderWxid, L"wxid_xxx2");
		swprintf_s(msg->content, L"阿是嘎尔尕为噶为我国仨人怪我怪我微软尕让他大啊的岗位打给我gasaegtaewagasgewateraasdgwatawbasrwafasdrewafsdfasd");

		std::thread(handleReceiveMsg, msg).detach();
		Sleep(500);
	}
}

/**
 * @brief hook接收消息
*/
void hookReceiveMsg() {
	hookAnyAddress(getWeChatWinDllBase() + WX_OFFSET_RECEIVE_MSG_HOOK, receiveMsgHook);

	// 测试
	//std::thread(testHandleReceiveMsg).detach();

}