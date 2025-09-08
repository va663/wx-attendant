#include "pch.h"
#include <stack>
#include <map>
#include <set>

std::set<std::wstring> contactExcludedSet{
	L"fmessage", L"qqmail",L"medianote",L"qmessage",L"newsapp",L"filehelper",L"weixin", L"tmessage", L"mphelper", L"floatbottle", L"qqsafe", L"gh_3dfda90e39d6", L"gh_7aac992b0363"
};
std::map < std::wstring, WxContactInfo > friendContactInfoMap;

/**
 * @brief 二叉树遍历拿到所有节点地址
*/
std::vector<DWORD> traversalBT(DWORD nodeAddr, UINT leftOffset, UINT rightOffset)
{
	std::vector<DWORD> nodeAddrs;
	std::stack<DWORD> st;
	while (nodeAddr || !st.empty()) {
		if (nodeAddr) {
			st.push(nodeAddr);
			nodeAddr = *(LPDWORD)(nodeAddr += leftOffset);
		}
		else {
			nodeAddr = st.top();
			nodeAddrs.push_back(nodeAddr);
			st.pop();
			nodeAddr = *(LPDWORD)(nodeAddr += rightOffset);
		}
	}
	return nodeAddrs;
}

/**
 * @brief 链表遍历拿到所有节点地址
*/
std::vector<DWORD> traversalLinkedList(DWORD nodeAddr) {
	std::vector<DWORD> nodeAddrs;

	DWORD flag = nodeAddr;
	do {
		nodeAddrs.push_back(nodeAddr);
	} while ((nodeAddr = *(LPDWORD)nodeAddr) != flag);

	return nodeAddrs;
}

/**
 * @brief 是否被exclude的联系人
 * @return
*/
BOOL isExcludedContactInfo(std::wstring wxid) {
	return (contactExcludedSet.find(wxid) != contactExcludedSet.end());
}

/**
 * @brief 微信账号类型推断
 * @param type1
 * @param type2
 * @return 0/无效 1/个人用户 3/群聊
*/
int guessContactType(std::wstring wxid, DWORD type1, DWORD type2) {
	SLOG_DEBUG(L"微信账号类型推断 - [wxid:{}, type1:{}, type2:{}]", wxid, type1, type2);
	// type1最后一位为1 且 type2为0 则表示个人用户
	if ((type1 & 0x01) == 1 && type2 == 0x0) {
		return 1;
	}
	// type1最后一位为0 且 type2为0 且 @chatroom结尾 则表示群
	else if ((type1 & 0x01) == 0 && type2 == 0x0 && Strs::endsWith(wxid, L"@chatroom")) {
		return 3;
	}
	else {
		// 其他
		return 0;
	}
}

/**
 * @brief 生成好友列表联系人map
*/
void buildFriendContactInfoMap() {
	try {
		std::vector<DWORD> nodeAddrs = traversalLinkedList(
			*(LPDWORD)((*(LPDWORD)(getWeChatWinDllBase() + WX_OFFSET_CONTACT_LIST)) + WX_OFFSET_CONTACT_LIST_NODE_ADDR)
		);
		SLOG_INFO(L"微信联系人获取 - 总共:{}个node", nodeAddrs.size());

		for (auto nodeAddr : nodeAddrs) {
			LPWSTR wxid = *(LPWSTR*)(nodeAddr + 0x30);
			if (IsBadReadPtr(wxid, 4) || Strs::len(wxid) == 0 || isExcludedContactInfo(wxid)) {
				continue;
			}

			DWORD type1 = *(PDWORD)(nodeAddr + 0x70);
			DWORD type2 = *(PDWORD)(nodeAddr + 0x74);
			DWORD type;
			if (!(type = guessContactType(wxid, type1, type2))) {
				// 忽略非个人用户和群聊
				continue;
			}

			LPWSTR wxcid = *(LPWSTR*)(nodeAddr + 0x44);
			if (IsBadReadPtr(wxcid, 4)) {
				wxcid = nullptr;
			}

			LPWSTR wxname = *(LPWSTR*)(nodeAddr + 0x8c);
			if (IsBadReadPtr(wxname, 4)) {
				wxname = nullptr;
			}

			LPWSTR wxremark = *(LPWSTR*)(nodeAddr + 0x78);
			if (IsBadReadPtr(wxremark, 4)) {
				wxremark = nullptr;
			}

			LPWSTR avatar = *(LPWSTR*)(nodeAddr + 0x130);
			if (IsBadReadPtr(avatar, 4)) {
				avatar = nullptr;
			}

			WxContactInfo contactInfo{ 0 };

			contactInfo.type = type;

			if (wxid) {
				swprintf_s(contactInfo.wxid, wxid);
			}

			if (wxcid) {
				swprintf_s(contactInfo.wxcid, wxcid);
			}

			if (wxname) {
				swprintf_s(contactInfo.wxname, wxname);
			}

			if (wxremark) {
				swprintf_s(contactInfo.wxremark, wxremark);
			}

			if (avatar) {
				swprintf_s(contactInfo.avatar, avatar);
			}

			SLOG_DEBUG(L"微信联系人获取 - [type:{}, wxid:{}, wxcid:{}, wxname:{}, wxremark:{}, avatar:{}]",
				contactInfo.type, contactInfo.wxid, contactInfo.wxcid, contactInfo.wxname, contactInfo.wxremark, contactInfo.avatar);

			friendContactInfoMap[contactInfo.wxid] = contactInfo;
		}
	}
	catch (std::exception& e) {
		SLOG_ERROR(u8"微信联系人获取失败, err:{}", e.what());
	}
	catch (...) {
		SLOG_ERROR(u8"微信联系人获取失败");
	}
}

/**
 * @brief 获取所有好友列表联系人信息（无序）
 * @return
*/
std::vector<WxContactInfo> getFriendContactInfoAll()
{
	std::vector<WxContactInfo> list;
	for (const auto& i : friendContactInfoMap) {
		list.push_back(i.second);
	}
	return list;
}

/**
 * @brief 获取好友列表联系人信息
 * @param wxid
 * @return
*/
BOOL getFriendContactInfo(std::wstring wxid, WxContactInfo& info) {
	if (wxid.empty()) {
		SLOG_ERROR(L"[getFriendContactInfo] - wxid is empty");
		return FALSE;
	}

	swprintf_s(info.wxid, wxid.data());

	auto i = friendContactInfoMap.find(wxid);

	if (i != friendContactInfoMap.end()) {
		if (!IsBadReadPtr(i->second.wxcid, 4)) {
			swprintf_s(info.wxcid, i->second.wxcid);
		}

		if (!IsBadReadPtr(i->second.wxname, 4)) {
			swprintf_s(info.wxname, i->second.wxname);
		}

		if (!IsBadReadPtr(i->second.wxremark, 4)) {
			swprintf_s(info.wxremark, i->second.wxremark);
		}

		if (!IsBadReadPtr(i->second.avatar, 4)) {
			swprintf_s(info.avatar, i->second.avatar);
		}

		return TRUE;
	}

	return FALSE;
}

/**
 * @brief 获取联系人信息
 * @param wxid
 * @return
*/
BOOL getContactInfo(std::wstring wxid, WxContactInfo& info) {
	if (wxid.empty()) {
		SLOG_ERROR(L"[getContactInfo] - wxid is empty");
		return FALSE;
	}
	//SLOG_DEBUG(L"微信联系人信息获取 - [wxid:{}]", wxid);

	DWORD contactList = *(LPDWORD)(getWeChatWinDllBase() + 0x222F3BC);
	DWORD call1 = getWeChatWinDllBase() + 0x701DC0;
	DWORD call2 = getWeChatWinDllBase() + 0x4024A0;

	WxStr id = { 0 };
	id.pStr = (LPWSTR)wxid.data();
	id.strLen = Strs::len(wxid.data());
	id.strLen2 = id.strLen;

	char infoBuf[0x3E8]{ 0 };

	int result = 0;
	__asm {
		pushad
		pushfd

		mov esi, contactList
		lea ebx, infoBuf
		push ebx

		sub esp, 0x14
		lea eax, id
		mov ecx, esp
		push eax
		call call1

		mov ecx, esi
		call call2

		mov result, eax

		popfd
		popad
	}

	swprintf_s(info.wxid, wxid.data());
	if (result) {
		DWORD type1 = *(PDWORD)(infoBuf + 0x50);
		DWORD type2 = *(PDWORD)(infoBuf + 0x54);
		info.type = guessContactType(wxid, type1, type2);

		PWCHAR wxcid = *(PWCHAR*)(infoBuf + 0x24);
		if (!IsBadReadPtr(wxcid, 4)) {
			swprintf_s(info.wxcid, wxcid);
		}

		PWCHAR wxname = *(PWCHAR*)(infoBuf + 0x6C);
		if (!IsBadReadPtr(wxname, 4)) {
			swprintf_s(info.wxname, wxname);
		}

		PWCHAR wxremark = *(PWCHAR*)(infoBuf + 0x58);
		if (!IsBadReadPtr(wxremark, 4)) {
			swprintf_s(info.wxremark, wxremark);
		}

		PWCHAR avatar = *(PWCHAR*)(infoBuf + 0x110);
		if (!IsBadReadPtr(avatar, 4)) {
			swprintf_s(info.avatar, avatar);
		}
	}

	return result;
}

/**
 * @brief 获取群信息
 * @param wxid
*/
BOOL getGroupInfo(std::wstring wxid, WxGroupInfo& info) {
	SLOG_DEBUG(L"微信群信息获取 - [wxid:{}]", wxid);

	DWORD bufBuild = getWeChatWinDllBase() + WX_OFFSET_GROUP_BUF_BUILD_CALL;
	DWORD getInfo = getWeChatWinDllBase() + WX_OFFSET_GROUP_INFO_GET_CALL;

	WxStr id = { 0 };
	id.pStr = (LPWSTR)wxid.data();
	id.strLen = Strs::len(wxid.data());
	id.strLen2 = id.strLen;

	char groupInfoBuf[0x160]{ 0 };

	int result = 0;
	__asm {
		pushad
		pushfd

		lea ecx, groupInfoBuf
		call bufBuild

		lea eax, groupInfoBuf
		push eax
		lea edi, id
		push edi
		call getInfo

		mov result, eax

		popfd
		popad
	}

	swprintf_s(info.wxid, wxid.data());
	if (result) {
		PWCHAR adminWxid = *(PWCHAR*)(groupInfoBuf + 76);
		if (!IsBadReadPtr(adminWxid, 4)) {
			swprintf_s(info.adminWxid, adminWxid);
		}

		PCHAR members = *(PCHAR*)(groupInfoBuf + 28);
		if (!IsBadReadPtr(members, 4)) {
			std::wstring ms = Strs::s2ws(members);
			info.members = Strs::split(ms, L"\\^G");
		}

		WxContactInfo contactInfo = { 0 };
		getContactInfo(wxid, contactInfo);
		if (!IsBadReadPtr(contactInfo.wxname, 4)) {
			swprintf_s(info.wxname, contactInfo.wxname);
		}
		if (!IsBadReadPtr(contactInfo.wxremark, 4)) {
			swprintf_s(info.wxremark, contactInfo.wxremark);
		}
		if (!IsBadReadPtr(contactInfo.avatar, 4)) {
			swprintf_s(info.avatar, contactInfo.avatar);
		}

		SLOG_DEBUG(L"微信群信息获取成功 - [wxid:{}, adminWxid:{}, wxname:{}, wxremark:{}, avatar:{}, memberCount:{}]",
			info.wxid, info.adminWxid, info.wxname, info.wxremark, info.avatar, info.members.size());
	}

	return result;
}