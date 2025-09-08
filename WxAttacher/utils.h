#pragma once

#include <string>
#include <time.h>
#include <sys/timeb.h>
#include <chrono>
#include <objbase.h>

#include "spdlog/spdlog.h"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h" // or "../stdout_sinks.h" if no color needed
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "Poco/Stopwatch.h"

class Strs
{

public:
	static std::string ws2s(std::wstring ws);
	static std::wstring s2ws(std::string s);

	static std::wstring dw2w(DWORD dw);

	static std::string utf82ansi(std::string s);
	static std::string ansi2utf8(std::string s);

	static size_t len(std::wstring s);

	static BOOL equalsIgnoreCase(std::wstring s1, std::wstring s2);
	static BOOL contains(std::wstring s1, std::wstring s2);
	static BOOL startsWith(std::wstring s1, std::wstring s2);
	static BOOL endsWith(std::wstring s1, std::wstring s2);

	static std::string replace(std::string& s, std::string& o, std::string& n);

	static std::vector<std::wstring> split(std::wstring& str, std::wstring delim);
};


class Base64
{
public:
	static std::string encode(const char* data, int len);
	static std::string encode(std::string s);

	static std::string decode(const char* data, int len, int& olen);
	static std::string decode(std::string s, int& olen);
};

class Files
{
public:

	static std::wstring getFileAbsoluteDir(LPWSTR filePath);
	static std::wstring getFilename(LPWSTR filePath);
};

class UUIDs
{
public:
	static std::wstring generate();
};

class Times
{
private:
	static LONG64 sntpInitMillis;
	static Poco::Stopwatch* pSntpTikcer;

public:
	static void restartSntpTicker();
	static int setSntp(LONG64 millis);

	static LONG64 currentLocalMillis();
	static LONG64 currentSntpMillis();
};

class Mems
{
public:
	static BOOL readMemory(
		DWORD pid,
		LPCVOID lpBaseAddress,
		LPVOID lpBuffer,
		SIZE_T nSize
	);

	static BOOL writeMemory(
		DWORD pid,
		LPCVOID lpBuffer,
		SIZE_T nSizee
	);

	static BOOL writeMemory(
		DWORD pid,
		LPVOID lpBaseAddress,
		LPCVOID lpBuffer,
		SIZE_T nSizee
	);
};

class Loggers
{
public:
	static Loggers* getInstance()
	{
		static Loggers xlogger;
		return &xlogger;
	}

	std::shared_ptr<spdlog::logger> getLogger()
	{
		return m_logger;
	}

private:
	// make constructor private to avoid outside instance
	Loggers()
	{
		// log path
		extern std::wstring ENV_EXE_ABSOLUTE_DIR;
		const std::string log_dir = Strs::utf82ansi(Strs::ws2s(ENV_EXE_ABSOLUTE_DIR + L"log"));
		const std::string logger_name_prefix = "WxAttacher_";

		// decide the log level
		std::string level = "debug";


		// logger name with timestamp
		int date = NowDateToInt();
		int time = NowTimeToInt();
		const std::string logger_name = logger_name_prefix + std::to_string(date); //+ "_" + std::to_string(time);

		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(log_dir + "/" + logger_name + ".log", 500 * 1024 * 1024, 100);

		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(console_sink);
		sinks.push_back(file_sink);
		m_logger = std::make_shared<spdlog::logger>(logger_name, sinks.begin(), sinks.end());

		// custom format
		m_logger->set_pattern("[%Y-%m-%dT%H:%M:%S.%e%z] [%l] %v");

		if (level == "trace")
		{
			m_logger->set_level(spdlog::level::trace);
			m_logger->flush_on(spdlog::level::trace);
		}
		else if (level == "debug")
		{
			m_logger->set_level(spdlog::level::debug);
			m_logger->flush_on(spdlog::level::debug);
		}
		else if (level == "info")
		{
			m_logger->set_level(spdlog::level::info);
			m_logger->flush_on(spdlog::level::info);
		}
		else if (level == "warn")
		{
			m_logger->set_level(spdlog::level::warn);
			m_logger->flush_on(spdlog::level::warn);
		}
		else if (level == "error")
		{
			m_logger->set_level(spdlog::level::err);
			m_logger->flush_on(spdlog::level::err);
		}
	}

	~Loggers()
	{
		spdlog::drop_all(); // must do this
	}

	void* operator new(size_t size)
	{}

	Loggers(const Loggers&) = delete;
	Loggers& operator=(const Loggers&) = delete;

private:
	static  int NowDateToInt()
	{
		time_t now;
		time(&now);

		// choose thread save version in each platform
		tm p;
#ifdef _WIN32
		localtime_s(&p, &now);
#else
		localtime_r(&now, &p);
#endif // _WIN32
		int now_date = (1900 + p.tm_year) * 10000 + (p.tm_mon + 1) * 100 + p.tm_mday;
		return now_date;
	}

	static  int NowTimeToInt()
	{
		time_t now;
		time(&now);
		// choose thread save version in each platform
		tm p;
#ifdef _WIN32
		localtime_s(&p, &now);
#else
		localtime_r(&now, &p);
#endif // _WIN32

		int now_int = p.tm_hour * 10000 + p.tm_min * 100 + p.tm_sec;
		return now_int;
	}

private:
	std::shared_ptr<spdlog::logger> m_logger;

};

// logger; use embedded macro to support file and line number
#define SLOG_TRACE(...) SPDLOG_LOGGER_CALL(Loggers::getInstance()->getLogger().get(), spdlog::level::trace, __VA_ARGS__)
#define SLOG_DEBUG(...) SPDLOG_LOGGER_CALL(Loggers::getInstance()->getLogger().get(), spdlog::level::debug, __VA_ARGS__)
#define SLOG_INFO(...) SPDLOG_LOGGER_CALL(Loggers::getInstance()->getLogger().get(), spdlog::level::info, __VA_ARGS__)
#define SLOG_WARN(...) SPDLOG_LOGGER_CALL(Loggers::getInstance()->getLogger().get(), spdlog::level::warn, __VA_ARGS__)
#define SLOG_ERROR(...) SPDLOG_LOGGER_CALL(Loggers::getInstance()->getLogger().get(), spdlog::level::err, __VA_ARGS__)
