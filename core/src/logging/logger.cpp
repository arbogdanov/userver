#include <userver/logging/logger.hpp>

#include <memory>

// this header must be included before any spdlog headers
// to override spdlog's level names
#include <logging/spdlog.hpp>

#include <logging/reopening_file_sink.hpp>

#include <spdlog/formatter.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_sinks.h>

#include "config.hpp"

USERVER_NAMESPACE_BEGIN

namespace logging {

namespace {

LoggerPtr MakeSimpleLogger(const std::string& name, spdlog::sink_ptr sink,
                           spdlog::level::level_enum level) {
  auto logger = std::make_shared<spdlog::logger>(name, sink);
  logger->set_formatter(std::make_unique<spdlog::pattern_formatter>(
      LoggerConfig::kDefaultPattern));
  logger->set_level(level);
  logger->flush_on(level);
  return logger;
}

spdlog::sink_ptr MakeStderrSink() {
  static auto sink = std::make_shared<spdlog::sinks::stderr_sink_mt>();
  return sink;
}

spdlog::sink_ptr MakeStdoutSink() {
  static auto sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
  return sink;
}

}  // namespace

LoggerPtr MakeStderrLogger(const std::string& name, Level level) {
  return MakeSimpleLogger(name, MakeStderrSink(),
                          static_cast<spdlog::level::level_enum>(level));
}

LoggerPtr MakeStdoutLogger(const std::string& name, Level level) {
  return MakeSimpleLogger(name, MakeStdoutSink(),
                          static_cast<spdlog::level::level_enum>(level));
}

LoggerPtr MakeFileLogger(const std::string& name, const std::string& path,
                         Level level) {
  return MakeSimpleLogger(name,
                          std::make_shared<logging::ReopeningFileSinkMT>(path),
                          static_cast<spdlog::level::level_enum>(level));
}

LoggerPtr MakeNullLogger(const std::string& name) {
  return MakeSimpleLogger(name, std::make_shared<spdlog::sinks::null_sink_mt>(),
                          spdlog::level::off);
}

namespace impl {

void LogRaw(Logger& logger, Level level, std::string_view message) {
  auto spdlog_level = static_cast<spdlog::level::level_enum>(level);
  logger.log(spdlog_level, "{}", message);
}

}  // namespace impl

}  // namespace logging

USERVER_NAMESPACE_END
