#include "pch.h"

#include <codecvt>
#include <atlconv.h>
#include <atlstr.h>
#include <regex>

std::string Strs::ws2s(std::wstring ws)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8_conv;
	return utf8_conv.to_bytes(ws);
}

std::wstring Strs::s2ws(std::string s)
{
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8_conv;
	return utf8_conv.from_bytes(s);
}

std::string Strs::utf82ansi(std::string s) {
	UINT nLen = MultiByteToWideChar(CP_UTF8, NULL, s.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_UTF8, NULL, s.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(CP_ACP, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_ACP, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	s = szBuffer;
	delete[]szBuffer;
	delete[]wszBuffer;
	return s;
}

std::string Strs::ansi2utf8(std::string s) {
	UINT nLen = MultiByteToWideChar(CP_ACP, NULL, s.c_str(), -1, NULL, NULL);
	WCHAR* wszBuffer = new WCHAR[nLen + 1];
	nLen = MultiByteToWideChar(CP_ACP, NULL, s.c_str(), -1, wszBuffer, nLen);
	wszBuffer[nLen] = 0;
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, NULL, NULL, NULL, NULL);
	CHAR* szBuffer = new CHAR[nLen + 1];
	nLen = WideCharToMultiByte(CP_UTF8, NULL, wszBuffer, -1, szBuffer, nLen, NULL, NULL);
	szBuffer[nLen] = 0;
	s = szBuffer;
	delete[]wszBuffer;
	delete[]szBuffer;
	return s;
}

std::wstring Strs::dw2w(DWORD dw) {
	return std::to_wstring(dw);
}

size_t Strs::len(std::wstring s) {
	return wcslen(s.data());
}

BOOL Strs::equalsIgnoreCase(std::wstring s1, std::wstring s2) {
	return lstrcmpi(s1.data(), s2.data()) == 0;
}

BOOL Strs::contains(std::wstring s1, std::wstring s2) {
	return s1.size() >= s2.size() && s1.find(s2) != std::wstring::npos;
}

BOOL Strs::startsWith(std::wstring s1, std::wstring s2) {
	return s1.size() >= s2.size() && s1.find(s2) == 0;
}

BOOL Strs::endsWith(std::wstring s1, std::wstring s2) {
	return s1.size() >= s2.size() && s1.rfind(s2) == (s1.size() - s2.size());
}

std::string Strs::replace(std::string& s, std::string& o, std::string& n)
{
	for (std::string::size_type pos(0); pos != std::string::npos; pos += n.length()) {
		if ((pos = s.find(o, pos)) != std::string::npos) {
			s.replace(pos, o.length(), n);
		}
		else {
			break;
		}
	}
	return s;
}

std::vector<std::wstring> Strs::split(std::wstring& str, std::wstring delim) {
	std::wregex re{ delim };
	return std::vector<std::wstring> {
		std::wcregex_token_iterator(str.data(), str.data() + str.length(), re, -1),
			std::wcregex_token_iterator()
	};
}

std::string Base64::encode(const char* data, int len) {
	const char EncodeTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string str;
	unsigned char tmp[4] = { 0 };
	int llen = 0;
	for (int i = 0; i < (int)(len / 3); i++) {
		tmp[1] = *data++;
		tmp[2] = *data++;
		tmp[3] = *data++;
		str += EncodeTable[tmp[1] >> 2];
		str += EncodeTable[((tmp[1] << 4) | (tmp[2] >> 4)) & 0x3F];
		str += EncodeTable[((tmp[2] << 2) | (tmp[3] >> 6)) & 0x3F];
		str += EncodeTable[tmp[3] & 0x3F];
		if (llen += 4, llen == 76) {
			str += "\r\n";
			llen = 0;
		}
	}
	int mod = len % 3;
	if (mod == 1) {
		tmp[1] = *data++;
		str += EncodeTable[(tmp[1] & 0xFC) >> 2];
		str += EncodeTable[((tmp[1] & 0x03) << 4)];
		str += "==";
	}
	else if (mod == 2) {
		tmp[1] = *data++;
		tmp[2] = *data++;
		str += EncodeTable[(tmp[1] & 0xFC) >> 2];
		str += EncodeTable[((tmp[1] & 0x03) << 4) | ((tmp[2] & 0xF0) >> 4)];
		str += EncodeTable[((tmp[2] & 0x0F) << 2)];
		str += "=";
	}
	return str;
}

std::string Base64::encode(std::string s) {
	return Base64::encode(s.c_str(), s.size());
}


std::string Base64::decode(const char* data, int len, int& olen) {
	const char DecodeTable[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			62,
			0, 0, 0,
			63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
			0, 0, 0, 0, 0, 0, 0,
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12,
			13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
			0, 0, 0, 0, 0, 0,
			26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38,
			39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,
	};
	std::string str;
	int index;
	int i = 0;
	while (i < len) {
		if (*data != '\r' && *data != '\n') {
			index = DecodeTable[*data++] << 18;
			index += DecodeTable[*data++] << 12;
			str += (index & 0x00FF0000) >> 16;
			olen++;
			if (*data != '=') {
				index += DecodeTable[*data++] << 6;
				str += (index & 0x0000FF00) >> 8;
				olen++;
				if (*data != '=') {
					index += DecodeTable[*data++];
					str += index & 0x000000FF;
					olen++;
				}
			}
			i += 4;
		}
		else {
			data++;
			i++;
		}
	}
	return str;
}

std::string Base64::decode(std::string s, int& olen)
{
	return Base64::decode(s.c_str(), s.size(), olen);
}


/**
 * @brief 获取路径的绝对目录 C:\\A\\B\\
 * @param filePath
 * @return
*/
std::wstring Files::getFileAbsoluteDir(LPWSTR filePath) {
	WCHAR drive[_MAX_DRIVE] = { 0 };
	WCHAR dir[_MAX_DIR] = { 0 };
	WCHAR fname[_MAX_FNAME] = { 0 };
	WCHAR ext[_MAX_EXT] = { 0 };
	_wsplitpath_s(filePath, drive, dir, fname, ext);
	std::wstring str;
	return str.append(drive).append(dir);
}

/**
 * @brief 获取路径的文件名 xx.xx
 * @param filePath
 * @return
*/
std::wstring Files::getFilename(LPWSTR filePath) {
	WCHAR drive[_MAX_DRIVE] = { 0 };
	WCHAR dir[_MAX_DIR] = { 0 };
	WCHAR fname[_MAX_FNAME] = { 0 };
	WCHAR ext[_MAX_EXT] = { 0 };
	_wsplitpath_s(filePath, drive, dir, fname, ext);
	std::wstring str;
	return str.append(fname).append(ext);
}

/**
 * @brief uuid生成
 * @return
*/
std::wstring UUIDs::generate() {
	char buf[64] = { 0 };
	GUID guid;

	if (CoCreateGuid(&guid)) {
		return Strs::s2ws(std::move(std::string("")));
	}

	sprintf_s(buf,
		"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2],
		guid.Data4[3], guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);

	return Strs::s2ws(std::move(std::string(buf)));
}

/**
 * @brief 获取本地当前毫秒时间戳
 * @return
*/
LONG64 Times::currentLocalMillis() {
	timeb t;
	ftime(&t);
	return t.time * 1000 + t.millitm;
}
