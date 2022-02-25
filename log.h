#ifndef LOG_H
#define LOG_H
#include <iostream>

#ifndef __FILENAME__
#define __FILENAME__ __FILE__
#endif

const char* const log_strings[] = { 
	"\033[35m[Debug] \033[m", 
	"\033[34m[CPU Info] \033[m", 
	"\033[33m[Warning] \033[m", 
	"\033[31m[Error] \033[m" };
#define LOG(level) \
if (level < Log::log_level) ; \
else Log::get_stream() << '[' << __FILENAME__ << ":" << std::dec << __LINE__ << "] "<< log_strings[level]


enum Level
{
	Debug,
	CPU_Info,
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