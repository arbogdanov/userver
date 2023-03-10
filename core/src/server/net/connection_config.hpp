#pragma once

#include <memory>
#include <string>

#include <userver/yaml_config/yaml_config.hpp>

#include <server/request/request_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace net {

struct ConnectionConfig {
  size_t in_buffer_size = 32 * 1024;
  size_t requests_queue_size_threshold = 100;
  std::chrono::seconds keepalive_timeout{10 * 60};

  // Actually required, wrapped in an optional to simplify parsing
  std::optional<request::RequestConfig> request;
};

ConnectionConfig Parse(const yaml_config::YamlConfig& value,
                       formats::parse::To<ConnectionConfig>);

}  // namespace net
}  // namespace server

USERVER_NAMESPACE_END
