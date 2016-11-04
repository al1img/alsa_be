/*
 *  Log implementation
 *  Copyright (c) 2016, Oleksandr Grytsov
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef SRC_XEN_LOG_HPP_
#define SRC_XEN_LOG_HPP_

#include <atomic>
#include <cstring>
#include <mutex>
#include <sstream>

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define LOG(instance, level) XenBackend::LogLine().get(instance, __FILENAME__, __LINE__, XenBackend::LogLevel::log ## level)

#ifndef NDEBUG

#define DLOG(instance, level) LOG(instance, level)

#else

#define DLOG(instance, level) \
	true ? (void) 0 : XenBackend::LogVoid() & LOG(instance, level)
#endif

namespace XenBackend {

enum class LogLevel
{
	logDISABLE, logERROR, logWARNING, logINFO, logDEBUG
};

class Log;

class LogVoid
{
public:
	LogVoid() { }
	void operator&(std::ostream&) { }
};

class Log
{
public:
	Log(const std::string& name, LogLevel level = sCurrentLevel, bool fileAndLine = sShowFileAndLine) :
		mName(name),  mLevel(level), mFileAndLine(fileAndLine) {}

	static void setLogLevel(LogLevel level) { sCurrentLevel = level; }
	static bool setLogLevel(const std::string& strLevel);

	static LogLevel getLogLevel() { return sCurrentLevel; }

	static void setShowFileAndLine(bool showFileAndLine) { sShowFileAndLine = showFileAndLine; }
	static bool getShowFileAndLine() { return sShowFileAndLine; }

private:

	friend class LogLine;

	static std::atomic<LogLevel> sCurrentLevel;
	static std::atomic_bool sShowFileAndLine;

	std::string mName;
	LogLevel mLevel;
	bool mFileAndLine;
};

class LogLine
{
public:
	LogLine();
	virtual ~LogLine();

	std::ostringstream& get(Log& log, const char* file, int line, LogLevel level = LogLevel::logDEBUG);
	std::ostringstream& get(const char* name, const char* file, int line, LogLevel level = LogLevel::logDEBUG);

private:
	static size_t sAlignmentLength;

	std::ostringstream mStream;
	LogLevel mCurrentLevel;
	LogLevel mSetLevel;

	std::mutex mMutex;

	void putHeader(const std::string& header);
	std::string nowTime();
	std::string levelToString(LogLevel level);
};

}

#endif /* SRC_XEN_LOG_HPP_ */
