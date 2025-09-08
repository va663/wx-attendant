#include "WebSocket.h"
#include "utils.h"

using Poco::Net::HTTPClientSession;
using Poco::Net::HTTPSClientSession;
using Poco::Net::HTTPRequest;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPMessage;
using Poco::Net::WebSocket;
using Poco::Net::SSLManager;
using Poco::Net::Context;
using Poco::Net::AcceptCertificateHandler;

/**
 * @brief deflateѹ�� base64����
 * @param text
 * @return
*/
std::string deflate(std::string& text) {
	std::ostringstream os;
	Poco::Base64Encoder b64os(os);
	Poco::DeflatingOutputStream deflater(b64os, Poco::DeflatingStreamBuf::STREAM_ZLIB);

	deflater << text;
	deflater.close();
	b64os.close();

	return os.str();
}

/**
 * @brief inflate��ѹ base64���
 * @param deflated
 * @return
*/
std::string inflate(std::string& deflated) {
	std::istringstream is(deflated);
	Poco::Base64Decoder b64is(is);
	Poco::InflatingInputStream inflater(b64is, Poco::InflatingStreamBuf::STREAM_ZLIB);

	std::string data(std::istreambuf_iterator<char>{inflater}, {});

	return data;
}

/**
 * @brief websocket����
*/
class WsKeepalive {
public:
	Poco::Net::WebSocket* pWs;

	WsKeepalive(WebSocket* pWs) :pWs(pWs) {}

	void heartbeat(Poco::Timer& t) {
		if (!pWs) {
			return;
		}
		try {
			SLOG_TRACE(u8"WebSocket[{}] ping...", pWs->peerAddress().toString());
			pWs->sendFrame(NULL, NULL, WebSocket::FRAME_FLAG_FIN | WebSocket::FRAME_OP_PING);
		}
		catch (...) {
			SLOG_ERROR(u8"WebSocket[{}] ping failed...", pWs->peerAddress().toString());
			pWs->shutdown();
		}
	}
};

/**
 * @brief websocket���ͺ���
 * @param pWs
 * @param msg
*/
void sendToWs(WebSocket* pWs, std::string msg, std::string encrypt) {
	if (!pWs || msg.empty()) {
		return;
	}
	try
	{
		std::string buf;

		if (encrypt == "compress") {
			buf = deflate(msg);
		}
		else {
			buf = msg;
		}

		pWs->sendFrame(buf.data(), buf.size(), WebSocket::FRAME_TEXT);
	}
	catch (std::exception& e) {
		SLOG_ERROR(u8"WebSocket[{}]������Ϣʧ��, msg:{}, err:{}", pWs->peerAddress().toString(), msg, e.what());
	}
	catch (...) {
		SLOG_ERROR(u8"WebSocket[{}]������Ϣʧ��, msg:{}", pWs->peerAddress().toString(), msg);
	}
}

/**
 * @brief ��һ��websocket����
 * @param closed
 * @param ppWs
 * @param host
 * @param port
 * @param uri
 * @param msgHandler
*/
void openWs(WebSocket** ppWs, std::string host, int port, std::string uri, std::string token, bool ssl, std::string encrypt, BOOL& closed, void(*openedCallback)(), void(*msgHandler)(std::string))
{
	std::string url;
	url.append(host).append(":").append(std::to_string(port)).append(uri);

	int retryCount = 0;
	int retryInterval = 5000;
	int heartbeatInterval = 30000;

	Poco::Timer heartbeatTimer(heartbeatInterval, heartbeatInterval);

	WebSocket* pWs = nullptr;
	while (!closed)
	{
		try {
			SLOG_INFO(u8"WebSocket[{}]���ӽ�����", url);

			if (ssl) {
				SSLManager::InvalidCertificateHandlerPtr handlerPtr(new AcceptCertificateHandler(false));
				Context::Ptr context = new Context(Context::TLSV1_1_CLIENT_USE, "");
				SSLManager::instance().initializeClient(nullptr, handlerPtr, context);

				HTTPSClientSession cs(host, port);
				HTTPRequest request(HTTPRequest::HTTP_GET, uri, HTTPMessage::HTTP_1_1);
				request.set(u8"token", token);
				request.set(u8"encrypt", encrypt);
				HTTPResponse response;

				pWs = new WebSocket(cs, request, response);
			}
			else {
				HTTPClientSession cs(host, port);
				HTTPRequest request(HTTPRequest::HTTP_GET, uri, HTTPMessage::HTTP_1_1);
				request.set(u8"token", token);
				request.set(u8"encrypt", encrypt);
				HTTPResponse response;

				pWs = new WebSocket(cs, request, response);
			}

			*ppWs = pWs;

			SLOG_INFO(u8"WebSocket[{}]���ӽ����ɹ�", url);
			if (openedCallback) {
				// ���ӳɹ���Ļص�
				openedCallback();
			}
			retryCount = 0;

			// ��������
			WsKeepalive keepalive(pWs);
			heartbeatTimer.setStartInterval(heartbeatInterval);
			heartbeatTimer.setPeriodicInterval(heartbeatInterval);
			heartbeatTimer.start(Poco::TimerCallback<WsKeepalive>(keepalive, &WsKeepalive::heartbeat));

			// ��Ϣ����
			int n = 0, flags = 0, opCode = 0;

			char buf[8192] = { 0 };
			int bufSzie = sizeof(buf);
			do
			{
				memset(buf, 0, bufSzie);
				n = pWs->receiveFrame(buf, bufSzie, flags);
				opCode = (flags & WebSocket::FRAME_OP_BITMASK);
				if (opCode == WebSocket::FRAME_OP_PONG)
				{
					SLOG_TRACE(u8"WebSocket[{}] pong...", url);
				}
				else if (opCode == WebSocket::FRAME_OP_TEXT)
				{
					std::string msg = encrypt == "compress" ? inflate(std::string(buf)) : buf;
					SLOG_TRACE(u8"WebSocket[{}]���յ���Ϣ, msg:[{}]", url, msg);
					msgHandler(msg);
				}
			} while (opCode != WebSocket::FRAME_OP_CLOSE);

			SLOG_WARN(u8"WebSocket[{}]���ӱ�Զ�̹ر�", url);
		}
		catch (std::exception& e) {
			SLOG_ERROR(u8"WebSocket[{}]����ʧ�ܣ�{}ms������:{}", url, retryInterval, e.what());
		}
		catch (...) {
			SLOG_ERROR(u8"WebSocket[{}]���ӷ���δ֪�����޷�����", url);
			throw;
		}

		heartbeatTimer.stop();

		delete pWs;
		pWs = nullptr;
		*ppWs = pWs;

		if (closed)
		{
			break;
		}

		Sleep(retryInterval);
		SLOG_INFO(u8"WebSocket[{}]��������:{}��", url, ++retryCount);
	}
	SLOG_INFO(u8"WebSocket[{}]���ӹر�", url);
}

WsSession::WsSession(std::string host, int port, std::string path, std::string token, bool ssl, std::string encrypt, void(*openedCallback)(), void(*msgHandler)(std::string)) :host(host), port(port), path(path), token(token), ssl(ssl), encrypt(encrypt) {
	this->url.append(host).append(":").append(std::to_string(port)).append(path);
	std::thread(openWs, &pWs, host, port, path, token, ssl, encrypt, std::ref(closed), openedCallback, msgHandler).detach();
}

void WsSession::sendMsg(std::string msg) {
	std::thread(sendToWs, this->pWs, msg, encrypt).detach();
}

bool WsSession::opened() {
	return !IsBadReadPtr(this->pWs, 4);
}

void WsSession::discard() {
	if (pWs) {
		pWs->shutdown();
	}
}

void WsSession::close() {
	this->closed = TRUE;
	if (pWs) {
		pWs->shutdown();
	}
}
