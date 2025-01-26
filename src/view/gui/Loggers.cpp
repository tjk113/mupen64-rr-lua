/*
 * Copyright (c) 2025, Mupen64 maintainers, contributors, and original authors (Hacktarux, ShadowPrince, linker).
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "stdafx.h"
#include "Loggers.h"
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

#include "core/services/LoggingService.h"

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_shared_logger;
std::shared_ptr<spdlog::logger> g_view_logger;

void Loggers::init()
{
    HANDLE h_file = CreateFile(L"mupen.log", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (h_file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER size{};
        GetFileSizeEx(h_file, &size);

        // Clear log file if bigger than 50MB
        if (size.QuadPart > 50ull * 1024ull * 1024ull)
        {
            SetFilePointerEx(h_file, {.QuadPart = 0}, nullptr, FILE_BEGIN);
            SetEndOfFile(h_file);
        }

        CloseHandle(h_file);
    }

#ifdef _DEBUG
    spdlog::sinks_init_list sink_list = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log"),
        std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>()
    };
#else
    spdlog::sinks_init_list sink_list = {
        std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log"),
    };
#endif
    
    g_core_logger = std::make_shared<spdlog::logger>("Core", sink_list);
    g_shared_logger = std::make_shared<spdlog::logger>("Shared", sink_list);
    g_view_logger = std::make_shared<spdlog::logger>("View", sink_list);
    
    g_core_logger->set_level(spdlog::level::trace);
    g_shared_logger->set_level(spdlog::level::trace);
    g_view_logger->set_level(spdlog::level::trace);
    g_core_logger->flush_on(spdlog::level::err);
    g_shared_logger->flush_on(spdlog::level::err);
    g_view_logger->flush_on(spdlog::level::err);
}
