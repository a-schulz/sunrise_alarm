#include "logger.h"

String *Logger::log_buffer = nullptr;
int Logger::log_index = 0;
bool Logger::buffer_full = false;
int Logger::max_entries = 0;

void Logger::init(int max_entries_count)
{
    max_entries = max_entries_count;
    log_buffer = new String[max_entries];
    log_index = 0;
    buffer_full = false;
}

void Logger::log(const String &message)
{
    if (log_buffer == nullptr)
        return;

    String timestamp = getTimestamp();
    log_buffer[log_index] = timestamp + message;
    log_index = (log_index + 1) % max_entries;

    if (log_index == 0)
    {
        buffer_full = true;
    }

    Serial.println(message);
}

String Logger::getLogsHtml()
{
    if (log_buffer == nullptr)
        return "";

    String html = "";
    int start_index = buffer_full ? log_index : 0;
    int count = buffer_full ? max_entries : log_index;

    for (int i = 0; i < count; i++)
    {
        int idx = (start_index + i) % max_entries;
        html += "<div class='log'>" + log_buffer[idx] + "</div>";
    }

    return html;
}

int Logger::getLogCount()
{
    return buffer_full ? max_entries : log_index;
}

String Logger::getTimestamp()
{
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", &timeinfo);
        return String(time_str) + " ";
    }
    return "";
}
