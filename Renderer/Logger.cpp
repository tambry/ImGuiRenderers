#include "Logger.h"

void log(LogLevel level, const std::string format, ...)
{
	va_list args;
	std::string fmt;

	switch (level)
	{
	case ERROR:   fmt += "E: ";  break;
	case INFO:    fmt += "I: ";  break;
	case WARNING: fmt += "W: ";  break;
	case DEBUG:   fmt += "D: ";  break;
	case NONE:    fmt += format; break;
	}

	va_start(args, format);

	if (level != NONE)
	{
		fmt += format + "\n";
	}

	std::vector<char> buffer(vsnprintf(nullptr, 0, fmt.c_str(), args) + 1);
	vsnprintf(buffer.data(), buffer.size(), fmt.c_str(), args);

	if (level == ERROR)
	{
		fprintf(stderr, buffer.data());
	}
	else
	{
		fprintf(stdout, buffer.data());
	}

	va_end(args);
}