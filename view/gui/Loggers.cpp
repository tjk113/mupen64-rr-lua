#include "Loggers.h"

#include <vector>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/wincolor_sink.h>

#include "shared/services/LoggingService.h"

std::shared_ptr<spdlog::logger> g_core_logger;
std::shared_ptr<spdlog::logger> g_shared_logger;
std::shared_ptr<spdlog::logger> g_view_logger;

void Loggers::init()
{
#ifdef _DEBUG
    std::vector<spdlog::sink_ptr> core_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    std::vector<spdlog::sink_ptr> shared_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    std::vector<spdlog::sink_ptr> view_sinks = {std::make_shared<spdlog::sinks::wincolor_stdout_sink_mt>(), std::make_shared<spdlog::sinks::basic_file_sink_mt>("mupen.log")};
    g_core_logger = std::make_shared<spdlog::logger>("Core", begin(core_sinks), end(core_sinks));
    g_shared_logger = std::make_shared<spdlog::logger>("Shared", begin(shared_sinks), end(shared_sinks));
    g_view_logger = std::make_shared<spdlog::logger>("View", begin(view_sinks), end(view_sinks));
#else
    g_core_logger = spdlog::basic_logger_mt("Core", "mupen.log");
    g_shared_logger = spdlog::basic_logger_mt("Shared", "mupen.log");
    g_view_logger = spdlog::basic_logger_mt("View", "mupen.log");
#endif
}
