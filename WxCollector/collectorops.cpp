#include "pch.h"

/**
 * @brief ���ս��̼�WM��Ϣ
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

		// ΢�ŵ�¼�û���Ϣ��ȡreq
		case WM_COPYDATA_EVENT_WX_USER_INFO_GET_REQ:
		{
			WxLoginInfo loginInfo = getLoginInfo();

			if (Strs::len(loginInfo.wxid)) {
				sendWmMsg(WM_COPYDATA_EVENT_WX_USER_INFO_GET_RESP, sizeof(WxLoginInfo), (PVOID)&loginInfo);
			}
		}
		break;

		// ��ϵ�˻�ȡreq
		case WM_COPYDATA_EVENT_WX_CONTACT_PULL_REQ:
		{
			buildFriendContactInfoMap();
			std::vector<WxContactInfo> contactList = getFriendContactInfoAll();
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_PULL_RESP, contactList.size() * sizeof(WxContactInfo), contactList.data());
		}
		break;

		// ��ϵ��ͬ��req
		case WM_COPYDATA_EVENT_WX_CONTACT_SYNC_REQ:
		{
			buildFriendContactInfoMap();
			std::vector<WxContactInfo> contactList = getFriendContactInfoAll();
			sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_SYNC_RESP, contactList.size() * sizeof(WxContactInfo), contactList.data());
		}
		break;

		// ������Ϣ
		case WM_COPYDATA_EVENT_WX_MSG_SEND:
		{
			WxSendTextMsg* msg = (WxSendTextMsg*)pCopyData->lpData;
			if (!loggedin()) {
				SLOG_ERROR(L"΢����Ϣ����ʧ�ܣ�δ��¼");
				return;
			}
			WxLoginInfo loginInfo = getLoginInfo();
			if (!Strs::equalsIgnoreCase(msg->fromWxid, loginInfo.wxid)) {
				SLOG_ERROR(L"΢����Ϣ����ʧ�ܣ���Ϣ���ͷ��뵱ǰ��¼�û���ƥ�䣬��ǰ΢�ŵ�¼�û�:[wxid:{}, wxcid:{}, wxname:{}], ��Ϣ:[fromWxid:{}, toWxid:{}, atWxid:{}, content:{}]",
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
		SLOG_ERROR(u8"receiveWmMsgʧ��:{}", e.what());
	}

}