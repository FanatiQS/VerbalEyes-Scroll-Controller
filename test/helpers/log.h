#ifndef LOG_H
#define LOG_H
#define LOGFLAGPRINT 1
#define LOGFLAGBUFFER 2
void log_setflags();
void log_cmp(const char*);
void log_clear();
#endif
