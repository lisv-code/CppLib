// ****** Logger. (c) 2024 LISV ******
#pragma once
#ifndef _LIS_LOGGER_H_
#define _LIS_LOGGER_H_

#include <chrono>
#include <vector>
#include <functional>
#include "FileSystem.h"

namespace LisLog {

enum LogLevel { llTrace = 0, llDebug = 1, llInfo = 2, llWarn = 3, llError = 4, llFault = 5, llNone = 6 };

extern const char* TimeFormat_String;
extern const char* TimeFormat_Milliseconds;

extern const char* TimeFormat_FileName;

// ***************************************** LogTargetBase *****************************************

class LogTargetBase
{
protected:
	int status;
	LogLevel logLevel;

	LogTargetBase(LogLevel lvl) : status(0), logLevel(lvl) { }
	virtual ~LogTargetBase() { }

	enum EventType { etGeneric, etSequel };
	struct LogEvent {
		EventType Type;
		LogLevel Level;
		std::chrono::system_clock::time_point Time;
		const char* Data;
	};

	virtual void WriteEvent(const LogEvent& evt) = 0;

	static int GetTimeStr(std::chrono::system_clock::time_point tp, char* buf, size_t len,
		const char* tm_fmt = TimeFormat_String, const char* ms_fmt = TimeFormat_Milliseconds);
	typedef std::function<void(const char* txt)> TextFunc;
	static void WriteEventAsText(const LogEvent& evt, TextFunc func);

	friend class Logger;
public:
	int GetStatus() const { return status; }
};

// *************************************** ILogger interface ***************************************

class ILogger
{
public:
	typedef LogTargetBase* TargetHandle;

	virtual LogLevel GetCurrentLogLevel() = 0;

	virtual TargetHandle AddTarget(LogTargetBase* target) = 0;
	virtual const LogTargetBase* GetTarget(int index) const = 0;
	virtual bool DelTarget(TargetHandle target) = 0;

	virtual void LogTxt(LogLevel lvl, const char* text) = 0;
	virtual int LogFmt(LogLevel lvl, const char* format, ...) = 0;
	virtual void LogHex(LogLevel lvl, const char* text, const unsigned char* data, size_t size) = 0;

	virtual ~ILogger() {};
};

// ************************************ ILogger implementation *************************************

struct LoggerSettings
{
	// general log settings
};

const int LogFmtMsgSizeMax = 0x26A0;

class Logger : public ILogger
{
	LoggerSettings settings;
	LogLevel lowestLogLevel;
	std::vector<LogTargetBase*> targets;
	std::chrono::system_clock::time_point lastEventTime; // TODO: value should depend on thread (+ class instance) //thread_local
	bool LogLvlChk(LogLevel event_level, LogLevel target_level);
	void WriteEvent(LogTargetBase::EventType typ, LogLevel lvl, const char* txt);

	Logger(LoggerSettings settings); // hide possibility to instantiate directly
public:
	static int InitSingleton(LoggerSettings settings, LogTargetBase* targets[], size_t target_count);
	static ILogger* GetInstance();

	virtual ~Logger();

	virtual LogLevel GetCurrentLogLevel() override;

	virtual TargetHandle AddTarget(LogTargetBase* target) override;
	virtual const LogTargetBase* GetTarget(int index) const override;
	virtual bool DelTarget(TargetHandle target) override;

	void LogTxt(LogLevel lvl, const char* text) override;
	int LogFmt(LogLevel lvl, const char* format, ...) override;
	void LogHex(LogLevel lvl, const char* text, const unsigned char* data, size_t size) override;
};

// ***************************************** LogTarget... ******************************************

class LogTargetDebugOut : public LogTargetBase
{
protected:
	virtual void WriteEvent(const LogEvent& evt) override;
public:
	LogTargetDebugOut(LogLevel lvl = llDebug) : LogTargetBase(lvl) { }
};

class LogTargetTextFunc : public LogTargetBase
{
	TextFunc function;
protected:
	virtual void WriteEvent(const LogEvent& evt) override;
public:
	LogTargetTextFunc(TextFunc func, LogLevel lvl = llInfo) : LogTargetBase(lvl) { function = func; }
};

class LogTargetTextFile : public LogTargetBase
{
	std::basic_string<FILE_PATH_CHAR> location, fileNamePrefix;
protected:
	virtual void WriteEvent(const LogEvent& evt) override;
public:
	LogTargetTextFile(const FILE_PATH_CHAR* location_path,
		const FILE_PATH_CHAR* file_name_prefix, LogLevel lvl = llInfo);
	virtual ~LogTargetTextFile();

	std::basic_string<FILE_PATH_CHAR> GetFilePath(std::chrono::system_clock::time_point time) const;
};

} // namespace LisLog

#endif // #ifndef _LIS_LOGGER_H_
