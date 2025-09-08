#include "pch.h"

/**
 * @brief 接收进程间WM消息
 * @param pCopyData
*/
void receiveWmMsg(COPYDATASTRUCT* pCopyData) {
	SLOG_DEBUG(L"WxCollector receiveWmMsg: event:[{}], content:[{}]", pCopyData->dwData, pCopyData->lpData);

	try {
		switch (pCopyData->dwData)
		{
		case WM_COPYDATA_EVENT_COLLECTOR_DETACH:
		{
			PostQuitMessage(0);
			ENV_MODULE_DETACHED = TRUE;
		}
		break;

		case WM_COPYDATA_EVENT_OPEN_COLLECTOR_CONSOLE:
		{
			openConsole();
			SLOG_DEBUG(L"WxCollector console opened");
		}
		break;

		case WM_COPYDATA_EVENT_CLOSE_COLLECTOR_CONSOLE:
		{
			freeConsole();
			SLOG_DEBUG(L"WxCollector console closed");
		}
		break;

		// 微信登录用户信息获取req
		case WM_COPYDATA_EVENT_WX_USER_INFO_GET_REQ:
		{
			WxLoginInfo loginInfo = getLoginInfo();

			if (Strs::len(loginInfo.wxid)) {
				sendWmMsg(WM_COPYDATA_EVENT_WX_USER_INFO_GET_RESP, sizeof(WxLoginInfo), (PVOID)&loginInfo);
			}
		}
		break;

		// 联系人获取req
		case WM_COPYDATA_EVENT_WX_CONTACT_PULL_REQ:
		{
			buildFriendContactInfoMap();
			std::vector<WxContactInfo> contactList = getFriendContactInfoAll();
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_PULL_RESP, contactList.size() * sizeof(WxContactInfo), contactList.data());
		}
		break;

		// 联系人同步req
		case WM_COPYDATA_EVENT_WX_CONTACT_SYNC_REQ:
		{
			buildFriendContactInfoMap();
			std::vector<WxContactInfo> contactList = getFriendContactInfoAll();
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_SYNC_RESP, contactList.size() * sizeof(WxContactInfo), contactList.data());
		}
		break;

		// 发送消息
		case WM_COPYDATA_EVENT_WX_MSG_SEND:
		{
			WxSendTextMsg* msg = (WxSendTextMsg*)pCopyData->lpData;
			if (!loggedin()) {
				SLOG_ERROR(L"微信消息发送失败，未登录");
				return;
			}
			WxLoginInfo loginInfo = getLoginInfo();
			if (!Strs::equalsIgnoreCase(msg->fromWxid, loginInfo.wxid)) {
				SLOG_ERROR(L"微信消息发送失败，消息发送方与当前登录用户不匹配，当前微信登录用户:[wxid:{}, wxcid:{}, wxname:{}], 消息:[fromWxid:{}, toWxid:{}, atWxid:{}, content:{}]",
					loginInfo.wxid, loginInfo.wxcid, loginInfo.wxname, msg->fromWxid, msg->toWxid, msg->atWxid, msg->content);
				return;
			}
			if (msg->toWxid && Strs::len(msg->toWxid) && msg->content && Strs::len(msg->content)) {
				sendTextMsg(msg->toWxid, msg->content, msg->atWxid);
			}
		}
		break;

		default:
			break;
		}
	}
	catch (std::exception& e) {
		SLOG_ERROR(u8"receiveWmMsg失败:{}", e.what());
	}

}