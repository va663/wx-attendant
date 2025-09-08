#include <strstream>
#include <iostream>

#include "WebSocket.h"
#include "WxAttacher.h"
#include "Json.h"

/**
 * @brief 消息上报的排除wxid
*/
std::set<std::wstring> cclExcludedSet;

/**
 * @brief 用于消息去重
*/
std::set<std::wstring> msgIdemSet;

/**
 * @brief WX_COLLECCTOR的PID
*/
DWORD wxColllectorPid;

/**
 * @brief WX_COLLECCTOR检测
*/
class WxCollectorInspector {
public:
	WxCollectorInspector() {}

	void inspect(Poco::Timer& t) {
		if (findWxCollectorWindow()) {
			SLOG_TRACE(u8"WX_COLLECCTOR inspect - attached");
		}
		else {
			SLOG_INFO(u8"WX_COLLECCTOR inspect - detached");

			// 清空微信登录信息
			wxLoginInfo = { 0 };

			JsonObject bodyL;
			bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_DETACH_OK);
			sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);

			discardRemoteWs();

			t.restart(0);
		}
	}
};

/**
 * @brief 查找正在运行的微信登录用户信息
 * @return
*/
std::map<DWORD, WxLoginInfo> findRunningWxLoginInfo() {
	std::vector<DWORD> pids = findPID(TEXT(WX_PROCESS_NAME));
	std::map<DWORD, WxLoginInfo> infos;

	for (auto pid : pids) {

	}
	return infos;
}

/**
 * @brief 附加WX_COLLECCTOR
 * @return
*/
int attachWxCollector(std::wstring wxid) {
	DWORD wxPid;
	if (!(wxPid = findWxPID(wxid))) {
		SLOG_ERROR(L"附加WX_COLLECCTOR - 指定微信未启动[wxid:{}]", wxid);
		return -1;
	}
	SLOG_INFO(L"附加WX_COLLECCTOR - 找到微信进程[wxid:{}, pid:{}]", wxid, wxPid);

	int attachResult = attach(wxPid, ENV_WX_COLLECTOR_DLL_PATH.data());
	if (attachResult == 0) {
		wxColllectorPid = wxPid;
	}

	return attachResult;
}

/**
 * @brief 卸载WX_COLLECCTOR
 * @return
*/
int detachWxCollector() {
	SLOG_INFO(L"卸载WX_COLLECCTOR - 微信进程[wxid:{}, pid:{}]", wxLoginInfo.wxid, wxColllectorPid);

	sendWmMsg(WM_COPYDATA_EVENT_COLLECTOR_DETACH, 0, NULL);
	Sleep(1000);

	return detach(wxColllectorPid, ENV_WX_COLLECTOR_DLL_PATH.data());
}

int inspectInterval = 2000;
Poco::Timer inspectorTimer(inspectInterval, inspectInterval);

/**
 * @brief 心跳检测WX_COLLECCTOR
*/
void inspectWxCollector() {
	inspectorTimer.stop();
	inspectorTimer.setStartInterval(inspectInterval);
	inspectorTimer.setPeriodicInterval(inspectInterval);
	inspectorTimer.start(Poco::TimerCallback<WxCollectorInspector>(WxCollectorInspector(), &WxCollectorInspector::inspect));
}

/**
 * @brief 是否拿到微信登录用户信息
 * @return
*/
boolean checkWxLoginInfo() {
	return wcslen(wxLoginInfo.wxid) > 0;
}

std::wstring sessionApproveId;
std::wstring tmpLastSntpId;

/**
 * @brief 执行一次sntp
*/
void fireSntp() {
	tmpLastSntpId = UUIDs::generate();
	sntpRemoteWs(tmpLastSntpId, clientLoginInfo.userId, L"USER");
}

/**
 * @brief 处理服务端授权成功事件
*/
void handleSessionApproved(std::wstring approvedId, std::wstring tenantId, std::wstring userId) {
	// 记录客户端当前登录用户信息
	swprintf_s(clientLoginInfo.tenantId, tenantId.data());
	swprintf_s(clientLoginInfo.userId, userId.data());
	SLOG_INFO(L"客户端登录用户 tenantId:{}, userId:{}, token:{}",
		clientLoginInfo.tenantId, clientLoginInfo.userId, wcslen(clientLoginInfo.token) ? L"assigned" : L"not assigned");

	// sntp
	fireSntp();

	sessionApproveId = approvedId;
	while (Strs::equalsIgnoreCase(sessionApproveId, approvedId)) {
		if (findWxCollectorWindow() && checkWxLoginInfo()) {
			// 告诉服务端WX_COLLECTOR已经attached
			JsonObject json;
			json.set("wxid", Strs::ws2s(wxLoginInfo.wxid));
			json.set("wxcid", Strs::ws2s(wxLoginInfo.wxcid));
			json.set("wxname", Strs::ws2s(wxLoginInfo.wxname));
			json.set("phone", Strs::ws2s(wxLoginInfo.phone));
			json.set("avatar", Strs::ws2s(wxLoginInfo.avatar));

			JsonObject bodyR;
			bodyR.set("event", "EVENT_WX_COLLECTOR_ATTACHED");
			bodyR.set("wxLoginInfo", json);
			sendToRemoteWs(clientLoginInfo.userId, L"USER", L"WX_COLLECTOR_SVC", L"SYSTEM", L"EVENT", bodyR);

			break;
		}
		Sleep(500);
	}
}

/**
 * @brief 接收进程间WM消息
 * @param pCopyData
*/
void receiveWmMsg(COPYDATASTRUCT* pCopyData) {
	//SLOG_DEBUG(L"WxAttacher receiveWmMsg: event:[{}], content:[{}]", pCopyData->dwData, pCopyData->lpData);

	try {
		switch (pCopyData->dwData)
		{

			// WX_COLLECTOR初始化
		case WM_COPYDATA_EVENT_COLLECTOR_INIT:
		{
			WxLoginInfo* pInfo = (WxLoginInfo*)pCopyData->lpData;
			if (wcslen(pInfo->wxid)) {
				SLOG_INFO(L"WX_COLLECTOR初始化成功 - 当前微信登录用户:[wxid:{}, wxcid:{}, wxname:{}, phone:{}, avatar:{}]"
					, pInfo->wxid, pInfo->wxcid, pInfo->wxname, pInfo->phone, pInfo->avatar);

				swprintf_s(wxLoginInfo.wxid, pInfo->wxid);
				swprintf_s(wxLoginInfo.wxcid, pInfo->wxcid);
				swprintf_s(wxLoginInfo.wxname, pInfo->wxname);
				swprintf_s(wxLoginInfo.phone, pInfo->phone);
				swprintf_s(wxLoginInfo.avatar, pInfo->avatar);

				JsonObject json;
				json.set("wxid", Strs::ws2s(wxLoginInfo.wxid));
				json.set("wxcid", Strs::ws2s(wxLoginInfo.wxcid));
				json.set("wxname", Strs::ws2s(wxLoginInfo.wxname));
				json.set("phone", Strs::ws2s(wxLoginInfo.phone));
				json.set("avatar", Strs::ws2s(wxLoginInfo.avatar));

				// 发送初始化成功消息到客户端
				JsonObject bodyL;
				bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_ATTACH_OK);
				bodyL.set("wxLoginInfo", json);
				sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);

				// 心跳检测
				inspectWxCollector();
			}
			else {
				SLOG_ERROR(L"WX_COLLECTOR初始化失败，无法获取微信登录用户，将卸载WX_COLLECTOR");
				// 发送初始化失败消息到客户端
				JsonObject bodyL;
				bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_ATTACH_FAILED);
				bodyL.set("errMsg", u8"WX_COLLECTOR初始化失败，无法获取微信登录用户，将卸载WX_COLLECTOR");
				sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);

				detachWxCollector();
			}
		}
		break;

		// NOTICE通知
		case WM_COPYDATA_EVENT_NOTICE:
		{
			SLOG_INFO((LPWSTR)pCopyData->lpData);
		}
		break;

		// 微信版本不支持
		case WM_COPYDATA_EVENT_WX_VERSION_NOT_SUPPORT:
		{
			std::vector<std::wstring> vs = Strs::split(std::wstring((PWCHAR)pCopyData->lpData), L"\\|");
			std::wstring v = vs[0];
			std::wstring sv = vs[1];

			SLOG_ERROR(L"当前微信版本({})不支持，请下载{}版本", v, sv);

			JsonObject json;
			json.set("curVersion", v);
			json.set("expectedVersion", sv);

			JsonObject bodyL;
			bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_ATTACH_FAILED);
			bodyL.set("cause", "WX_VERSION_NOT_SUPPORT");
			bodyL.set("extra", json);

			sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);

			detachWxCollector();
		}
		break;

		// 微信退出登录
		case WM_COPYDATA_EVENT_WX_USER_LOGOUT:
		{
			SLOG_INFO(L"微信退出登录 - [wxid:{}, wxname:{}]", wxLoginInfo.wxid, wxLoginInfo.wxname);

			JsonObject bodyL;
			bodyL.set("event", LOCALWS_EVENT_WX_LOGOUT);
			sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);

			detachWxCollector();
		}
		break;

		// 微信登录用户信息获取resp
		case WM_COPYDATA_EVENT_WX_USER_INFO_GET_RESP:
		{
			WxLoginInfo* pInfo = (WxLoginInfo*)pCopyData->lpData;

			if (wcslen(pInfo->wxid)) {
				swprintf_s(wxLoginInfo.wxid, pInfo->wxid);
				swprintf_s(wxLoginInfo.wxcid, pInfo->wxcid);
				swprintf_s(wxLoginInfo.wxname, pInfo->wxname);
				swprintf_s(wxLoginInfo.phone, pInfo->phone);
				swprintf_s(wxLoginInfo.avatar, pInfo->avatar);

				JsonObject json;
				json.set("wxid", Strs::ws2s(wxLoginInfo.wxid));
				json.set("wxcid", Strs::ws2s(wxLoginInfo.wxcid));
				json.set("wxname", Strs::ws2s(wxLoginInfo.wxname));
				json.set("phone", Strs::ws2s(wxLoginInfo.phone));
				json.set("avatar", Strs::ws2s(wxLoginInfo.avatar));

				JsonObject body;
				body.set("wxLoginInfo", json);
				sendToRemoteWs(clientLoginInfo.userId, L"USER", L"WX_LOGIN_INFO_SVC", L"SYSTEM", L"CUSTOM", body);
			}
		}
		break;

		// 消息上报
		case WM_COPYDATA_EVENT_WX_MSG_CAPTURE:
		{
			if (!checkWxLoginInfo()) {
				SLOG_ERROR(u8"微信消息上报失败 - 微信登录信息为空");
				break;
			}

			WxReceiveMsg* msg = (WxReceiveMsg*)pCopyData->lpData;

			// 不上报的wxid
			if (
				(cclExcludedSet.find(msg->senderWxid) != cclExcludedSet.end())
				||
				(cclExcludedSet.find(msg->groupWxid) != cclExcludedSet.end())
				) {
				break;
			}

			//SLOG_DEBUG(L"收到微信消息 - [msgType:{}, msgTypeDesc:{}, senderWxid:{}, senderWxcid:{}, senderWxname:{}, senderWxremark:{}, senderDisplayName:{}, recipientWxid:{}, recipientWxname:{}, groupWxid:{}, groupWxname:{}, groupWxremark:{}, atWxid:{}, content:{}]",
			//	msg->msgType, msg->msgTypeDesc, msg->senderWxid, msg->senderWxcid, msg->senderWxname, msg->senderWxremark, msg->senderDisplayName, msg->recipientWxid, msg->recipientWxname, msg->groupWxid, msg->groupWxname, msg->groupWxremark, msg->atWxid, msg->content);

			JsonObject body;
			body.set("senderWxid", Strs::ws2s(msg->senderWxid));
			body.set("senderWxcid", Strs::ws2s(msg->senderWxcid));
			body.set("senderWxname", Strs::ws2s(msg->senderWxname));
			body.set("senderWxremark", Strs::ws2s(msg->senderWxremark));
			body.set("senderAvatar", Strs::ws2s(msg->senderAvatar));
			body.set("senderDisplayName", Strs::ws2s(msg->senderDisplayName));
			body.set("inFriends", msg->inFriends ? true : false);
			body.set("recipientWxid", Strs::ws2s(msg->recipientWxid));
			body.set("recipientWxcid", Strs::ws2s(msg->recipientWxcid));
			body.set("recipientWxname", Strs::ws2s(msg->recipientWxname));
			body.set("recipientAvatar", Strs::ws2s(msg->recipientAvatar));
			body.set("groupWxid", Strs::ws2s(msg->groupWxid));
			body.set("groupWxname", Strs::ws2s(msg->groupWxname));
			body.set("groupWxremark", Strs::ws2s(msg->groupWxremark));
			body.set("groupAvatar", Strs::ws2s(msg->groupAvatar));
			body.set("atWxid", Strs::ws2s(msg->atWxid));
			body.set("content", Strs::ws2s(msg->content));

			sendToRemoteWs(clientLoginInfo.userId, L"USER", L"INQUIRY_SVC", L"IQ", L"TEXT", body);
		}
		break;

		// 联系人获取resp
		case WM_COPYDATA_EVENT_WX_CONTACT_PULL_RESP:
		{
			DWORD size = pCopyData->cbData;
			WxContactInfo* pInfo = (WxContactInfo*)pCopyData->lpData;

			SLOG_INFO(L"获取到联系人列表, total:[{}]", size / sizeof(WxContactInfo));

			for (int i = 0; i < size / sizeof(WxContactInfo); i++)
			{
				WxContactInfo curInfo = *(pInfo + i);
				SLOG_DEBUG(L"微信联系人信息 - [type:{}, wxid:{}, wxcid:{}, wxname:{}, wxremark:{}, avatar:{}]",
					curInfo.type, curInfo.wxid, curInfo.wxcid, curInfo.wxname, curInfo.wxremark, curInfo.avatar);
			}

		}
		break;

		// 联系人同步resp
		case WM_COPYDATA_EVENT_WX_CONTACT_SYNC_RESP:
		{
			DWORD size = pCopyData->cbData;
			WxContactInfo* pInfo = (WxContactInfo*)pCopyData->lpData;

			SLOG_INFO(L"联系人同步获取到联系人列表, total:[{}]", size / sizeof(WxContactInfo));
			JsonArray jsonArr;
			for (int i = 0; i < size / sizeof(WxContactInfo); i++)
			{
				WxContactInfo curInfo = *(pInfo + i);
				JsonObject json;
				json.set("wxid", Strs::ws2s(curInfo.wxid));
				json.set("type", curInfo.type);
				json.set("wxcid", Strs::ws2s(curInfo.wxcid));
				json.set("wxname", Strs::ws2s(curInfo.wxname));
				json.set("wxremark", Strs::ws2s(curInfo.wxremark));
				json.set("avatar", Strs::ws2s(curInfo.avatar));

				jsonArr.add(json);
			}
			JsonObject body;
			body.set("contactList", jsonArr);

			sendToRemoteWs(clientLoginInfo.userId, L"USER", L"USER_CONTACT_SYNC_SVC", L"SYSTEM", L"CUSTOM", body);
		}
		break;

		// 聊天窗口切换
		case WM_COPYDATA_EVENT_WX_CHAT_CONTACT_SWITCH:
		{
			WxContactInfo contactInfo = *(WxContactInfo*)pCopyData->lpData;

			//SLOG_DEBUG(L"微信聊天窗口切换 - [type:{}, wxid:{}, wxcid:{}, wxname:{}, wxremark:{}, avatar:{}]",
			//	contactInfo.type, contactInfo.wxid, contactInfo.wxcid, contactInfo.wxname, contactInfo.wxremark, contactInfo.avatar);

			JsonObject json;
			json.set("wxid", Strs::ws2s(contactInfo.wxid));
			json.set("wxcid", Strs::ws2s(contactInfo.wxcid));
			json.set("wxname", Strs::ws2s(contactInfo.wxname));
			json.set("wxremark", Strs::ws2s(contactInfo.wxremark));
			json.set("avatar", Strs::ws2s(contactInfo.avatar));

			JsonObject body;
			body.set("event", LOCALWS_EVENT_WX_CHAT_CONTACT_SWITCH);
			body.set("contactInfo", json);

			sendToLocalWs(LOCALWS_BODYTYPE_EVENT, body);
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

/**
 * @brief 接收本地ws消息
 * @param msg
{
	"deliveryId": "uuid",
	"deliveryTime": 1651214111000,
	"bodyType": "EVENT",
	"body": {
		"event": "EVENT_WX_CHAT_CONTACT_SWITCH",
		"wxid": "txxxxxxxxxxxxxt"
	}
}
*/
void receiveFromLocalWs(std::string wsMsg) {
	SLOG_TRACE(u8"receiveFromLocalWs:[{}]", wsMsg);

	try {
		JsonParser parser;
		JsonObjectPtr jsonObject = parser.parse(wsMsg).extract<JsonObjectPtr>();

		std::wstring deliveryId = jsonObject->getValue<std::wstring>("deliveryId");
		std::wstring bodyType = jsonObject->getValue<std::wstring>("bodyType");
		JsonObjectPtr bodyObject = jsonObject->get("body").extract<JsonObjectPtr>();

		if (Strs::equalsIgnoreCase(bodyType, TEXT(LOCALWS_BODYTYPE_CMD))) {
			// bodyType = CMD
			std::wstring cmd = bodyObject->getValue<std::wstring>("cmd");
			if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_OPEN_ATTACHER_CONSOLE))) {
				SLOG_INFO(L"打开ATTACHER CONSOLE");
				//openConsole();
				HWND hWnd = findWxAttacherDebugConsoleWindow();
				ShowWindow(hWnd, SW_SHOW);
				UpdateWindow(hWnd);
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_CLOSE_ATTACHER_CONSOLE))) {
				SLOG_INFO(L"关闭ATTACHER CONSOLE");
				//freeConsole();
				HWND hWnd = findWxAttacherDebugConsoleWindow();
				ShowWindow(hWnd, SW_HIDE);
				UpdateWindow(hWnd);
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_OPEN_COLLECTOR_CONSOLE))) {
				SLOG_INFO(L"打开COLLECTOR CONSOLE");
				sendWmMsg(WM_COPYDATA_EVENT_OPEN_COLLECTOR_CONSOLE, 0, NULL);
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_CLOSE_COLLECTOR_CONSOLE))) {
				SLOG_INFO(L"关闭COLLECTOR CONSOLE");
				sendWmMsg(WM_COPYDATA_EVENT_CLOSE_COLLECTOR_CONSOLE, 0, NULL);
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_ATTACH_COLLECTOR))) {
				SLOG_INFO(L"注入WX_COLLECCTOR");
				int attachResult = attachWxCollector(L"");
				if (attachResult == 2) {
					SLOG_INFO(L"卸载并重新注入WX_COLLECCTOR");
					detachWxCollector();
					Sleep(1000);
					attachResult = attachWxCollector(L"");
				}
				if (attachResult != 0) {
					SLOG_ERROR(L"WX_COLLECTOR attach失败");
					JsonObject bodyL;
					bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_ATTACH_FAILED);
					if (attachResult == -1) {
						bodyL.set("cause", "WX_NOT_RUNNING");
					}
					sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);
				}
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_DETACH_COLLECTOR))) {
				SLOG_INFO(L"卸载WX_COLLECCTOR");
				int detachResult = detachWxCollector();
				if (detachResult != 0) {
					SLOG_ERROR(L"WX_COLLECTOR detach失败");
					JsonObject bodyL;
					bodyL.set("event", LOCALWS_EVENT_WX_COLLECTOR_DETACH_FAILED);
					if (detachResult == -1) {
						bodyL.set("cause", "WX_NOT_RUNNING");
					}
					sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);
				}
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_CLOSE_ATTACHER_COLLECTOR))) {
				SLOG_INFO(L"关闭WX_ATTACHER&WX_COLLECCTOR");
				detachWxCollector();
				PostQuitMessage(0);
				closeRemoteWs();
				closeLocalWs();

				Sleep(500);
				exit(0);
			}
			else if (Strs::equalsIgnoreCase(cmd, TEXT(LOCALWS_CMD_CHECK_STATUS_REQ))) {
				SLOG_TRACE(L"检查状态");

				extern WsSession* remoteWs;
				bool wxConnectionAlright = findWxCollectorWindow() && wcslen(wxLoginInfo.wxid) ? true : false;
				bool serverConnectionAlright = (remoteWs && remoteWs->opened() && wcslen(clientLoginInfo.userId)) ? true : false;

				JsonObject bodyL;
				bodyL.set("ackId", Strs::ws2s(deliveryId));
				bodyL.set("cmd", LOCALWS_CMD_CHECK_STATUS_RESP);
				bodyL.set("wxAttacherPid", getpid());
				bodyL.set("wxConnectionAlright", wxConnectionAlright);
				bodyL.set("serverConnectionAlright", serverConnectionAlright);

				JsonObject json1;
				json1.set("wxid", Strs::ws2s(wxLoginInfo.wxid));
				json1.set("wxcid", Strs::ws2s(wxLoginInfo.wxcid));
				json1.set("wxname", Strs::ws2s(wxLoginInfo.wxname));
				json1.set("phone", Strs::ws2s(wxLoginInfo.phone));
				json1.set("avatar", Strs::ws2s(wxLoginInfo.avatar));

				JsonObject json2;
				json2.set("tenantId", Strs::ws2s(clientLoginInfo.tenantId));
				json2.set("userId", Strs::ws2s(clientLoginInfo.userId));

				bodyL.set("wxLoginInfo", json1);
				bodyL.set("clientLoginInfo", json2);

				sendToLocalWs(LOCALWS_BODYTYPE_CMD, bodyL, false);
			}

		}
	}
	catch (std::exception& e) {
		SLOG_ERROR(u8"receiveFromLocalWs失败:{}", e.what());
	}
}


/**
 * @brief 接收远程ws消息
{
	"deliveryId": "uuid",
	"deliveryTime": 1651214111000,
	"fromId": "fromfofm",
	"fromType": "USER",
	"toId": "tooo",
	"toType": "USER",
	"channelType": "WECHAT",
	"bodyType": "TEXT",
	"body": {

	}
}
 * @param msg
*/
void receiveFromRemoteWs(std::string wsMsg) {
	SLOG_INFO(u8"receiveFromRemoteWs:[{}]", wsMsg);

	try {
		JsonParser parser;
		JsonObjectPtr jsonObject = parser.parse(wsMsg).extract<JsonObjectPtr>();

		std::wstring deliveryId = jsonObject->getValue<std::wstring>("deliveryId");
		LONG64 deliveryTime = jsonObject->getValue<LONG64>("deliveryTime");

		std::wstring channelType = jsonObject->getValue<std::wstring>("channelType");

		std::wstring fromId = jsonObject->getValue<std::wstring>("fromId");
		std::wstring fromType = jsonObject->getValue<std::wstring>("fromType");

		std::wstring bodyType = jsonObject->getValue<std::wstring>("bodyType");
		JsonObjectPtr bodyObject = jsonObject->get("body").extract<JsonObjectPtr>();

		if (Strs::equalsIgnoreCase(channelType, L"WECHAT")) {

			if (Strs::equalsIgnoreCase(fromType, L"SYSTEM") && Strs::equalsIgnoreCase(fromId, L"SESSION_SVC")) {
				if (!Strs::equalsIgnoreCase(bodyType, L"SESSION_EVENT")) {
					SLOG_ERROR(u8"消息bodyType不支持:[{}]", wsMsg);
					return;
				}

				std::wstring event = bodyObject->getValue<std::wstring>("event");
				if (Strs::equalsIgnoreCase(event, L"EVENT_SESSION_APPROVED")) {
					// session approved
					std::wstring tenantId = bodyObject->getValue<std::wstring>("tenantId");
					std::wstring userId = bodyObject->getValue<std::wstring>("userId");
					std::thread(handleSessionApproved, deliveryId, tenantId, userId).detach();
				}
				else if (Strs::equalsIgnoreCase(event, L"EVENT_SESSION_AUTH_FAILED")) {
					// session鉴权失败
					SLOG_ERROR(L"WebSocket Remote 鉴权失败 - [EVENT_SESSION_AUTH_FAILED]");
					JsonObject bodyL;
					bodyL.set("event", LOCALWS_EVENT_SERVER_AUTH_FAILED);
					sendToLocalWs(LOCALWS_BODYTYPE_EVENT, bodyL);
				}
			}
			else if (Strs::equalsIgnoreCase(fromType, L"SYSTEM") && Strs::equalsIgnoreCase(fromId, L"SNTP_SVC")) {
				// SNTP响应
				std::wstring sntpAckId = bodyObject->getValue<std::wstring>("ackId");
				if (Strs::equalsIgnoreCase(tmpLastSntpId, sntpAckId)) {
					if (Times::setSntp(deliveryTime) == 1) {
						fireSntp();
					}
				}
			}
			else if (Strs::equalsIgnoreCase(fromType, L"SYSTEM") && Strs::equalsIgnoreCase(fromId, L"WX_LOGIN_INFO_SVC")) {
				// 微信登陆用户获取
				sendWmMsg(WM_COPYDATA_EVENT_WX_USER_INFO_GET_REQ, 0, NULL);
			}
			else if (Strs::equalsIgnoreCase(fromType, L"SYSTEM") && Strs::equalsIgnoreCase(fromId, L"WX_COLLECTOR_SVC")) {
				// 一些WX_COLLECTOR的操作
				JsonArrayPtr cclExcluded = bodyObject->get("ccl").extract<JsonObjectPtr>()->get("excluded").extract<JsonArrayPtr>();

				cclExcludedSet.clear();
				for (int i = 0; i < cclExcluded->size(); i++) {
					cclExcludedSet.insert(Strs::s2ws(cclExcluded->get(i).toString()));
				}

				SLOG_INFO(L"cclExcluded count:{}", cclExcludedSet.size());
			}
			else if (Strs::equalsIgnoreCase(fromType, L"SYSTEM") && Strs::equalsIgnoreCase(fromId, L"USER_CONTACT_SYNC_SVC")) {
				// 联系人同步
				sendWmMsg(WM_COPYDATA_EVENT_WX_CONTACT_SYNC_REQ, 0, NULL);
			}
			else if (Strs::equalsIgnoreCase(fromType, L"IQ") && Strs::equalsIgnoreCase(fromId, L"QUOTE_SVC")) {
				// 报价消息
				if (msgIdemSet.find(deliveryId) != msgIdemSet.end()) {
					SLOG_WARN(u8"消息重复投递", wsMsg);
					return;
				}

				if (!Strs::equalsIgnoreCase(bodyType, L"TEXT")) {
					SLOG_ERROR(u8"消息bodyType不支持:[{}]", wsMsg);
					return;
				}
				if (!checkWxLoginInfo()) {
					SLOG_ERROR(u8"微信消息发送失败 - 微信登录信息为空:[{}]", wsMsg);
					return;
				}

				std::wstring senderWxid = bodyObject->getValue<std::wstring>("senderWxid");
				if (!Strs::equalsIgnoreCase(senderWxid, wxLoginInfo.wxid)) {
					SLOG_ERROR(L"消息senderWxid不匹配当前微信登录用户, 当前微信登录用户:[wxid:{}, wxcid:{}, wxname:{}], 消息:[{}]",
						wxLoginInfo.wxid, wxLoginInfo.wxcid, wxLoginInfo.wxname, Strs::s2ws(wsMsg));
					return;
				}

				std::wstring recipientWxid = bodyObject->getValue<std::wstring>("recipientWxid");
				std::wstring atWxid = bodyObject->getValue<std::wstring>("atWxid");
				std::wstring content = bodyObject->getValue<std::wstring>("content");

				// 组装消息
				WxSendTextMsg* msg = new WxSendTextMsg{ 0 };
				swprintf_s(msg->fromWxid, senderWxid.data());
				swprintf_s(msg->toWxid, recipientWxid.data());
				swprintf_s(msg->atWxid, atWxid.data());
				swprintf_s(msg->content, content.data());

				// 记录用以去重
				msgIdemSet.insert(deliveryId);

				// 发送 联调阶段群聊暂不报价
				if (!Strs::endsWith(msg->toWxid, L"@chatroom")) {
					sendWmMsg(WM_COPYDATA_EVENT_WX_MSG_SEND, sizeof(WxSendTextMsg), (PVOID)msg);
				}
				delete msg;
			}
		}

	}
	catch (std::exception& e) {
		SLOG_ERROR(u8"receiveFromRemoteWs失败:{}", e.what());
	}

}