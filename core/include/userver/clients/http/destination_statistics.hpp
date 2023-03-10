#pragma once

#include <memory>
#include <unordered_map>

#include <userver/rcu/rcu_map.hpp>

#include <userver/clients/http/statistics.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients {
namespace http {

class DestinationStatistics final {
 public:
  // Return pointer to related RequestStats
  std::shared_ptr<RequestStats> GetStatisticsForDestination(
      const std::string& destination);

  // If max_auto_destinations reached, return nullptr, pointer to related
  // RequestStats otherwise
  std::shared_ptr<RequestStats> GetStatisticsForDestinationAuto(
      const std::string& destination);

  void SetAutoMaxSize(size_t max_auto_destinations);

  using DestinationsMap = rcu::RcuMap<std::string, Statistics>;

  DestinationsMap::ConstIterator begin() const;
  DestinationsMap::ConstIterator end() const;

 private:
  std::shared_ptr<RequestStats> GetExistingStatisticsForDestination(
      const std::string& destination);

  std::shared_ptr<RequestStats> CreateStatisticsForDestination(
      const std::string& destination);

 private:
  rcu::RcuMap<std::string, Statistics> rcu_map_;
  size_t max_auto_destinations_{0};
  std::atomic<size_t> current_auto_destinations_{0};
};

}  // namespace http
}  // namespace clients

USERVER_NAMESPACE_END
