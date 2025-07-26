// ****** Logger. (c) 2024-2025 LISV ******
#include "Logger.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include "StrUtils.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace LisLog {

const char* TimeFormat_String = "%Y-%m-%d %H:%M:%S";
const char* TimeFormat_Milliseconds = ".%03u";
const int TimeTextMaxLen = 23;

const char* TimeFormat_FileName = "%Y%m%d.log";

const auto EvtTimeNone = std::chrono::system_clock::time_point(std::chrono::system_clock::duration::zero());
const char* LogLvlStr[LogLevel::llNone] = { "trc", "dbg", "inf", "Wrn", "ERR", "FLT" };
const int LogLvlTxtMaxLen = 3;

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

Logger::~Logger() { }

bool Logger::LogLvlChk(LogLevel event_level, LogLevel target_level)
{
	return (event_level >= 0) && (event_level < LogLevel::llNone) && (event_level >= target_level);
}

void Logger::WriteEvent(LogTargetBase::EventType typ, LogLevel lvl, const char* txt)
{
	bool is_gen = LogTargetBase::EventType::etGeneral == typ;
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
	targets.emplace_back(target);
	if (target->logLevel < lowestLogLevel) lowestLogLevel = target->logLevel;
	return target;
}

const LogTargetBase* Logger::GetTarget(int index) const
{
	if ((index >= 0) && (index < targets.size())) {
		return targets[index].get();
	}
	return nullptr;
}

bool Logger::DelTarget(TargetHandle target)
{
	auto item = std::find_if(targets.begin(), targets.end(),
		[&target](const std::unique_ptr<LogTargetBase>& item) { return item.get() == target; });
	if (item != targets.end()) {
		targets.erase(item);
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
	WriteEvent(LogTargetBase::EventType::etGeneral, lvl, text);
}

int Logger::LogFmt(LogLevel lvl, const char* format, ...)
{
	static thread_local char txt_buf[LogMsgTxtMaxLen];

	if (!LogLvlChk(lvl, lowestLogLevel)) return 0;
	int result;
	std::va_list args;
	va_start(args, format);
	result = std::vsnprintf(txt_buf, sizeof(txt_buf), format, args);
	va_end(args);
	WriteEvent(LogTargetBase::EventType::etGeneral, lvl, txt_buf);
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
		WriteEvent(LogTargetBase::EventType::etSubsequent, lvl, buffer);
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
		result += std::snprintf(buf + result, len - result, ms_fmt, ms);
	}
	return result;
}

void LogTargetBase::WriteEventAsText(const LogEvent& evt, TextWriteFunc func, bool add_newline)
{
	const int TxtBufLen = TimeTextMaxLen + LogLvlTxtMaxLen + 4 + LogMsgTxtMaxLen + 2; // 4 - LogLvl brackets and spaces, 2 - '\n' & '\0'
	static thread_local char txt_buf[TxtBufLen];

	char* buf_pos = txt_buf;
	size_t buf_len = sizeof(txt_buf) - 2; // Reserve place for '\n' & '\0'
	if ((LogTargetBase::EventType::etGeneral == evt.Type) && (EvtTimeNone != evt.Time)) {
		int res = GetTimeStr(evt.Time, buf_pos, buf_len);
		buf_pos += res;  buf_len -= res;
		res = std::snprintf(buf_pos, buf_len, " [%s] ", LogLvlStr[evt.Level]);
		buf_pos += res;  buf_len -= res;
	}
	size_t data_size = std::min<size_t>(buf_len, std::strlen(evt.Data));
	std::memcpy(buf_pos, evt.Data, data_size);
	buf_pos += data_size;
	if (add_newline) {
		*buf_pos = '\n';
		++buf_pos;
	}
	*buf_pos = 0;
	func(evt.Type, txt_buf);
}

// *************************************** LogTargetDebugOut ***************************************

void LogTargetDebugOut::WriteEvent(const LogEvent& evt)
{
	WriteEventAsText(evt, [](EventType type, const char* txt) {
#ifdef _WINDOWS
		OutputDebugStringA(txt);
#endif
		// std::clog << txt;
	}, true);
}

// *************************************** LogTargetTextFunc ***************************************

void LogTargetTextFunc::WriteEvent(const LogEvent& evt)
{
	if (function)
		WriteEventAsText(evt, function, msgAddNewline);
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
	// TODO: probably some optmization needed & file concurrent (multithread) access
	std::ofstream ofs(GetFilePath(evt.Time).c_str(),
		std::ios_base::out | std::ios::binary | std::ios_base::app);
	WriteEventAsText(evt, [&ofs](EventType type, const char* txt) { ofs << txt; }, true);
	ofs.close();
}

} // namespace LisLog
