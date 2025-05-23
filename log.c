#include "./log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define FILE_LOCATION_LENGTH 512
#define LOG_MESSAGE_LENGTH 3040

#define ESC_CODE "\e["
#define REGULAR "0"
#define BOLD "1"
#define RED ";31m"    // ERROR && FATAL
#define YELLOW ";33m" // WARN
#define BLUE ";34m"   // INFO
#define GREEN ";32m"  // Debug
#define RESET "\e[0m"

typedef struct {
  FILE *file;
} LogFile;

static LogFile *log_file_instance = NULL;
static LogSettings *settings = NULL;

int setup_logs(LogSettings *log_settings) {
  time_t current_time = time(NULL);
  struct tm *local_time = localtime(&current_time);
  char *logfile_location = (char *)malloc(FILE_LOCATION_LENGTH * sizeof(char));

  char formatted_time[64];
  char *datetime_format = malloc(sizeof(char) * 8);

  if (log_settings->interval == Never) {
    snprintf(logfile_location, FILE_LOCATION_LENGTH, "./logs/chip-8%s.log", "");
  } else {
    if (log_settings->interval == Monthly) {
      datetime_format = "%Y-%m";
    } else if (log_settings->interval == Yearly) {
      datetime_format = "%Y";
    } else {
      datetime_format = "%F";
    }

    strftime(formatted_time, sizeof(formatted_time), datetime_format,
             local_time);
    snprintf(logfile_location, FILE_LOCATION_LENGTH, "./logs/chip-8_%s.log",
             formatted_time);
  }

  log_file_instance = (LogFile *)malloc(sizeof(LogFile));
  log_file_instance->file = fopen(logfile_location, "a");
  settings = log_settings;
  return 0;
}

int logevent(LogLevel loglevel, size_t max_size, const char *format, ...) {
  char *loglevel_string = (char *)malloc(sizeof(char) * 8);
  char *ansi_color = (char *)malloc(sizeof(char) * 16);
  char *message = malloc(sizeof(char) * max_size);

  va_list args;
  va_start(args, format);
  vsnprintf(message, max_size, format, args);
  va_end(args);

  switch (loglevel) {
  case Debug:
    loglevel_string = "Debug";
    snprintf(ansi_color, 16, "%s%s%s", ESC_CODE, REGULAR, GREEN);
    break;
  case Info:
    loglevel_string = "Info";
    snprintf(ansi_color, 16, "%s%s%s", ESC_CODE, REGULAR, BLUE);
    break;
  case Warn:
    loglevel_string = "Warn";
    snprintf(ansi_color, 16, "%s%s%s", ESC_CODE, REGULAR, YELLOW);
    break;
  case Error:
    loglevel_string = "Error";
    snprintf(ansi_color, 16, "%s%s%s", ESC_CODE, REGULAR, RED);
    break;
  case Fatal:
    loglevel_string = "Fatal";
    snprintf(ansi_color, 16, "%s%s%s", ESC_CODE, BOLD, RED);
    break;
  }

  if (log_file_instance == NULL && log_file_instance->file == NULL) {
    perror("Could not open Log file");
    return -1;
  }

  char *log_message_file = (char *)malloc(sizeof(char) * LOG_MESSAGE_LENGTH);
  char *log_message_console = (char *)malloc(sizeof(char) * LOG_MESSAGE_LENGTH);

  time_t current_time = time(NULL);
  struct tm *local_time = localtime(&current_time);
  char *datetime_format = "%Y-%m-%d %H:%M:%S";
  char formatted_time[64];

  strftime(formatted_time, sizeof(formatted_time), datetime_format, local_time);

  snprintf(log_message_file, 3040, "%s [%s] %s\n", formatted_time,
           loglevel_string, message);
  snprintf(log_message_console, 3040, "%s%s [%s]%s %s\n", ansi_color,
           formatted_time, loglevel_string, RESET, message);

  if (loglevel >= settings->minimum_file_log_level) {
    fputs(log_message_file, log_file_instance->file);
  }

  if (loglevel >= settings->minimum_console_log_level) {
    printf("%s", log_message_console);
  }

  free(ansi_color);
  free(log_message_file);
  free(log_message_console);
  free(message);

  return 0;
};

int close_log() {
  fclose(log_file_instance->file);
  free(log_file_instance);
  return 0;
}
