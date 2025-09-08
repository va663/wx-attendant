#pragma once

#pragma warning(disable:4996)

#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ws2_32.lib")

#include "Poco/UnicodeConverter.h"
#include "Poco/Timer.h"

#include <Poco/DeflatingStream.h>
#include <Poco/InflatingStream.h>

#include <Poco/Base64Encoder.h>
#include <Poco/Base64Decoder.h>

#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/HTTPMessage.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/SSLManager.h"
#include "Poco/Net/X509Certificate.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Net/PrivateKeyPassphraseHandler.h"
#include "Poco/Net/AcceptCertificateHandler.h"

#include "Json.h"

/////////////////////
//// EVENT
#define LOCALWS_BODYTYPE_EVENT "EVENT"

#define LOCALWS_EVENT_SERVER_AUTH_FAILED "EVENT_SERVER_AUTH_FAILED"

#define LOCALWS_EVENT_WX_COLLECTOR_ATTACH_OK "EVENT_WX_COLLECTOR_ATTACH_OK"
#define LOCALWS_EVENT_WX_COLLECTOR_ATTACH_FAILED "EVENT_WX_COLLECTOR_ATTACH_FAILED"
#define LOCALWS_EVENT_WX_COLLECTOR_DETACH_OK "EVENT_WX_COLLECTOR_DETACH_OK"
#define LOCALWS_EVENT_WX_COLLECTOR_DETACH_FAILED "EVENT_WX_COLLECTOR_DETACH_FAILED"

#define LOCALWS_EVENT_WX_CHAT_CONTACT_SWITCH "EVENT_WX_CHAT_CONTACT_SWITCH"

#define LOCALWS_EVENT_WX_LOGIN "EVENT_WX_LOGIN"
#define LOCALWS_EVENT_WX_LOGOUT "EVENT_WX_LOGOUT"

//// CMD
#define LOCALWS_BODYTYPE_CMD "CMD"

#define LOCALWS_CMD_OPEN_ATTACHER_CONSOLE "CMD_OPEN_ATTACHER_CONSOLE"
#define LOCALWS_CMD_CLOSE_ATTACHER_CONSOLE "CMD_CLOSE_ATTACHER_CONSOLE"
#define LOCALWS_CMD_OPEN_COLLECTOR_CONSOLE "CMD_OPEN_COLLECTOR_CONSOLE"
#define LOCALWS_CMD_CLOSE_COLLECTOR_CONSOLE "CMD_CLOSE_COLLECTOR_CONSOLE"
#define LOCALWS_CMD_CLOSE_ATTACHER_COLLECTOR "CMD_CLOSE_ATTACHER_COLLECTOR"
#define LOCALWS_CMD_ATTACH_COLLECTOR "CMD_ATTACH_COLLECTOR"
#define LOCALWS_CMD_DETACH_COLLECTOR "CMD_DETACH_COLLECTOR"
#define LOCALWS_CMD_CHECK_STATUS_REQ "CMD_CHECK_STATUS_REQ"
#define LOCALWS_CMD_CHECK_STATUS_RESP "CMD_CHECK_STATUS_RESP"

/////////////////////

/**
 * @brief websocket»á»°
*/
class WsSession {
private:
	BOOL closed = FALSE;
	std::string encrypt;

	std::string host;
	int port;
	std::string path;
	std::string token;
	bool ssl = false;
	std::string url;

	Poco::Net::WebSocket* pWs = nullptr;

public:

	WsSession(std::string host, int port, std::string path, std::string token, bool ssl, std::string encrypt, void(*openedCallback)(), void(*msgHandler)(std::string));

	void sendMsg(std::string msg);

	bool opened();

	void discard();
	void close();

	~WsSession()
	{
		this->close();
	}
};

void discardLocalWs();
void closeLocalWs();
void sendToLocalWs(std::string bodyType, JsonObject body, bool log = true);

void discardRemoteWs();
void closeRemoteWs();
void sendToRemoteWs(
	std::wstring fromId, std::wstring fromType,
	std::wstring toId, std::wstring toType,
	std::wstring bodyType, JsonObject body,
	bool log = true
);

void sntpRemoteWs(
	std::wstring sntpId,
	std::wstring fromId, std::wstring fromType
);
