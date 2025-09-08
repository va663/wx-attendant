// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{

	switch (ul_reason_for_call)
	{

	case DLL_PROCESS_ATTACH:
	{
		initModuleEnv(hModule);

		//std::wstring msg;
		//msg.append(ENV_MOUDLE_PATH).append(L" attached ok");
		//MessageBox(0, msg.c_str(), L"attacher", MB_OK);

		//DWORD weChatWinDllBase = getWeChatWinDllBase();
		//char* wxid = (char*)(weChatWinDllBase + WX_OFFSET_WXID);
		//MessageBox(0, Strs::a2w(wxid), L"attacher", 0);

		std::thread(init, hModule).detach();

		//·¢ËÍÏûÏ¢²âÊÔ
		//sendTextMsg((wchar_t*)L"22222@chatroom", (wchar_t*)L"¹þ¹þ¹þasgwaegaewawefagdhi iÅ¶iÅ¶ºðºðºðºðºðºðºðºð", (wchar_t*)L"wxid_xxxxx");
	}
	break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
	{
		FreeConsole();
		hookRecoveryAll();
		spdlog::drop_all();
		//SLOG_INFO(L"×¢Ïú:{}", ENV_MOUDLE_PATH);
	}
	break;
	}
	return TRUE;
}


