#ifndef Log_Interval

#define Log_Interval
#include <stddef.h>
typedef enum { Never, Yearly, Monthly, Daily } LogIntervals;
typedef enum { Debug, Info, Warn, Error, Fatal } LogLevel;

typedef struct {
  LogLevel minimum_file_log_level;
  LogLevel minimum_console_log_level;
  LogIntervals interval;
} LogSettings;

int setup_logs(LogSettings *log_settings);
int logevent(LogLevel loglevel, size_t max_size, const char *format, ...);
int close_log();

#endif
