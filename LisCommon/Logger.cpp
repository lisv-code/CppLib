// ****** Logger. (c) 2024 LISV ******
#include "Logger.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include "StrUtils.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace LisLog {

const char* TimeFormat_String = "%Y-%m-%d %H:%M:%S";
const char* TimeFormat_Milliseconds = ".%03u";

const char* TimeFormat_FileName = "%Y%m%d.log";

const auto EvtTimeNone = std::chrono::system_clock::time_point(std::chrono::system_clock::duration::zero());
const char* LogLvlStr[LogLevel::llNone] = { "trc", "dbg", "inf", "Wrn", "ERR", "FLT" };
const char* MsgHdrFmtStr = "%s [%s] ";

const LoggerSettings default_logger_settings = { };

static ILogger* logger_singleton = nullptr; // Global singleton

int Logger::InitSingleton(LoggerSettings settings, LogTargetBase* targets[], size_t target_count)
{
	if (logger_singleton) {
		delete logger_singleton;
		logger_singleton = nullptr;
	}
	logger_singleton = new Logger(settings);
	for (size_t i = 0; i < target_count; ++i) logger_singleton->AddTarget(targets[i]);
	return 0;
}

ILogger* Logger::GetInstance()
{
	if (!logger_singleton)
		InitSingleton(default_logger_settings, nullptr, 0); // is it necessary?
	return logger_singleton;
}

Logger::Logger(LoggerSettings settings)
{
	this->lowestLogLevel = llNone;
	this->settings = settings;
	this->lastEventTime = EvtTimeNone;
}

Logger::~Logger()
{
	for (auto& target : targets) delete target;
}

bool Logger::LogLvlChk(LogLevel event_level, LogLevel target_level)
{
	return (event_level >= 0) && (event_level < LogLevel::llNone) && (event_level >= target_level);
}

void Logger::WriteEvent(LogTargetBase::EventType typ, LogLevel lvl, const char* txt)
{
	bool is_gen = LogTargetBase::EventType::etGeneric == typ;
	LogTargetBase::LogEvent evt;
	evt.Type = typ;
	evt.Level = lvl;
	evt.Time = is_gen ? std::chrono::system_clock::now() : lastEventTime;
	evt.Data = txt;
	if (is_gen) lastEventTime = evt.Time;
	for (auto& target : targets) {
		if (LogLvlChk(evt.Level, target->logLevel)) target->WriteEvent(evt);
	}
}

// *************************************** ILogger interface ***************************************

LogLevel Logger::GetCurrentLogLevel()
{
	return lowestLogLevel;
}

ILogger::TargetHandle Logger::AddTarget(LogTargetBase* target)
{
	if (!target) return nullptr;
	targets.push_back(target);
	if (target->logLevel < lowestLogLevel) lowestLogLevel = target->logLevel;
	return target;
}

const LogTargetBase* Logger::GetTarget(int index) const
{
	if ((index >= 0) && (index < targets.size())) {
		return targets[index];
	}
	return nullptr;
}

bool Logger::DelTarget(TargetHandle target)
{
	auto item = std::find(targets.begin(), targets.end(), target);
	if (item != targets.end()) {
		targets.erase(item);
		delete static_cast<LogTargetBase*>(target);
		LogLevel lvl = llNone;
		for (const auto& target : targets) {
			if (target->logLevel < lvl) lvl = target->logLevel;
		}
		lowestLogLevel = lvl;
		return true;
	}
	return false;
}

void Logger::LogTxt(LogLevel lvl, const char* text)
{
	if (!LogLvlChk(lvl, lowestLogLevel)) return;
	WriteEvent(LogTargetBase::EventType::etGeneric, lvl, text);
}

int Logger::LogFmt(LogLevel lvl, const char* format, ...)
{
	if (!LogLvlChk(lvl, lowestLogLevel)) return 0;
	int result;
	va_list args;
	va_start(args, format);
	char txtFmtBuf[LogFmtMsgSizeMax];
	result = vsnprintf(txtFmtBuf, sizeof(txtFmtBuf), format, args);
	va_end(args);
	WriteEvent(LogTargetBase::EventType::etGeneric, lvl, txtFmtBuf);
	return result;
}

void Logger::LogHex(LogLevel lvl, const char* text, const unsigned char* data, size_t size)
{
	if (!LogLvlChk(lvl, lowestLogLevel)) return;
	LogFmt(lvl, "%s, %10.10zd bytes", text, size); // LogTargetBase::EventType::etGeneric
	const unsigned int width = 0x10;
	char buffer[0xFF];
	char* buf_pos = buffer;
	size_t buf_len = sizeof(buffer);
	unsigned int i, c, prn_res;
	for (i = 0; i < size; i += width) {
		prn_res = snprintf(buf_pos, buf_len, "%4.4x: ", i);
		if (prn_res >= 0) { buf_len -= prn_res; buf_pos += prn_res; }
		else break; // stop if error

		// show hex to the left
		for (c = 0; c < width; ++c) {
			if (i + c < size) prn_res = snprintf(buf_pos, buf_len, "%02x ", data[i + c]);
			else { for (int j = 0; j < 3; ++j) buf_pos[j] = ' '; prn_res = 3; }
			if (prn_res >= 0) { buf_len -= prn_res; buf_pos += prn_res; }
			else break; // stop if error
		}

		// show data on the right
		for (c = 0; (c < width) && (i + c < size); ++c) {
			*buf_pos = (data[i + c] >= 0x20 && data[i + c] < 0x80) ? data[i + c] : '.';
			--buf_len;
			if (0 >= buf_len) break;
			++buf_pos;
		}

		*buf_pos = 0;
		WriteEvent(LogTargetBase::EventType::etSequel, lvl, buffer);
		buf_pos = buffer;
		buf_len = sizeof(buffer);
	}
}

// ***************************************** LogTargetBase *****************************************

int LogTargetBase::GetTimeStr(std::chrono::system_clock::time_point tp,
	char* buf, size_t len, const char* tm_fmt, const char* ms_fmt)
{
	auto time = std::chrono::system_clock::to_time_t(tp);
	std::tm tm = *std::localtime(&time);
	int result = std::strftime(buf, len, tm_fmt, &tm);
	if (ms_fmt) {
		auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
		result += snprintf(buf + result, len - result, ms_fmt, ms);
	}
	return result;
}

void LogTargetBase::WriteEventAsText(const LogEvent& evt, TextFunc func)
{
	if ((LogTargetBase::EventType::etGeneric == evt.Type) && (EvtTimeNone != evt.Time)) {
		char buf[0xFF];
		int res = GetTimeStr(evt.Time, buf, sizeof(buf));
		snprintf(buf + res, sizeof(buf) - res, " [%s] ", LogLvlStr[evt.Level]);
		func(buf);
	}
	func(evt.Data);
	func("\n");
}

// *************************************** LogTargetDebugOut ***************************************

void LogTargetDebugOut::WriteEvent(const LogEvent& evt)
{
	WriteEventAsText(evt, [](const char* txt) {
#ifdef _WINDOWS
		OutputDebugStringA(txt);
#endif
		// std::clog << txt;
	});
}

// *************************************** LogTargetTextFunc ***************************************

void LogTargetTextFunc::WriteEvent(const LogEvent& evt)
{
	if (function)
		WriteEventAsText(evt, function);
}

// *************************************** LogTargetTextFile ***************************************

LogTargetTextFile::LogTargetTextFile(const FILE_PATH_CHAR* location_path,
	const FILE_PATH_CHAR* file_name_prefix, LogLevel lvl)
	: LogTargetBase(lvl)
{
	status = -1;
	if (LisFileSys::DirExistCheck(NULL, location_path, true)) {
		location = location_path;
		if (FILE_PATH_SEPARATOR_CHR != location[location.length() - 1])
			location += FILE_PATH_SEPARATOR_CHR;
		status = 0;
		if (file_name_prefix) fileNamePrefix = file_name_prefix;
	}
}

LogTargetTextFile::~LogTargetTextFile() { }

std::basic_string<FILE_PATH_CHAR> LogTargetTextFile::GetFilePath(
	std::chrono::system_clock::time_point time) const
{
	char buf[0xFF];
	GetTimeStr(time, buf, sizeof(buf), TimeFormat_FileName, nullptr);
	std::basic_string<FILE_PATH_CHAR> result = location;
	result += fileNamePrefix;
	result += (FILE_PATH_CHAR*)LisStr::CStrConvert(buf);
	return result;
}

void LogTargetTextFile::WriteEvent(const LogEvent& evt)
{
	// TODO: optmization needed & file concurrent (multithread) access
	std::ofstream ofs(GetFilePath(evt.Time).c_str(),
		std::ios_base::out | std::ios::binary | std::ios_base::app);
	WriteEventAsText(evt, [&ofs](const char* txt) { ofs << txt; });
	ofs.close();
}

} // namespace LisLog
