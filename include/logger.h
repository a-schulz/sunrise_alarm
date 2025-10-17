#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <time.h>

class Logger
{
public:
    static void init(int max_entries);
    static void log(const String &message);
    static String getLogsHtml();
    static int getLogCount();

private:
    static String *log_buffer;
    static int log_index;
    static bool buffer_full;
    static int max_entries;

    static String getTimestamp();
};

#define WEB_LOG(x) Logger::log(x)

#endif
