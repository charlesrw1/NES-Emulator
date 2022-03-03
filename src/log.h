#ifndef LOG_H
#define LOG_H
#include <iostream>

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

const char* const log_strings[] = { 
	"\033[35m[Debug] \033[m", 
	"", 
	"\033[34m[PPU Info] \033[m",
	"\033[32m[Info] \033[m",
	"\033[33m[Warning] \033[m", 
	"\033[31m[Error] \033[m" };
const char* const log_strings_no_color[] = {
	"[Debug] ",
	"[CPU Info] ",
	"[PPU Info] ",
	"[Info] ",
	"[Warning] ",
	"[Error] " };
#define COLOR_LOG

#ifdef COLOR_LOG
#define LOG(level) \
if (level < Log::log_level) ; \
else Log::get_stream() << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "<< log_strings[level]
#else
#define LOG(level) \
if (level < Log::log_level) ; \
else Log::get_stream() << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "<< log_strings_no_color[level]
#endif // COLOR_LOG

enum Level
{
	Debug,
	CPU_Info,
	PPU_Info,
	Info,
	Warning,
	Error 
};
class Log
{
public:
	static void set_stream(std::ostream* new_stream) {
		stream = new_stream;
	}
	static std::ostream& get_stream() {
		return *stream;
	}
	static Level log_level;
private:
	static std::ostream* stream;
};

#endif //LOG_H