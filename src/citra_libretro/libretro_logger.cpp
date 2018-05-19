// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include "common/logging/text_formatter.h"
#include "common/assert.h"

#include "libretro_logger.h"

LibRetroLogger::LibRetroLogger(retro_log_printf_t callback) : callback(callback) {}

const char *LibRetroLogger::GetName() const {
    return "LibRetro";
}

void LibRetroLogger::Write(const Log::Entry &entry) {
    retro_log_level log_level;

    switch (entry.log_level) {
        case Log::Level::Trace:
            log_level = retro_log_level::RETRO_LOG_DEBUG;
            break;
        case Log::Level::Debug:
            log_level = retro_log_level::RETRO_LOG_DEBUG;
            break;
        case Log::Level::Info:
            log_level = retro_log_level::RETRO_LOG_INFO;
            break;
        case Log::Level::Warning:
            log_level = retro_log_level::RETRO_LOG_WARN;
            break;
        case Log::Level::Error:
            log_level = retro_log_level::RETRO_LOG_ERROR;
            break;
        case Log::Level::Critical:
            log_level = retro_log_level::RETRO_LOG_ERROR;
            break;
        default:
            UNREACHABLE();
    }

    const char* class_name = GetLogClassName(entry.log_class);

    auto str = fmt::format("{} @ {}:{}:{}: {}\n", class_name, entry.filename, entry.function, entry.line_num,
                       entry.message);

    callback(log_level, str.c_str());
}
