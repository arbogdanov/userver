#pragma once

#include <chrono>

USERVER_NAMESPACE_BEGIN

namespace congestion_control {

/// Abstract sensor that fetches counters. These counters will be used
/// to forecast saturation levels and limit current load.
class Sensor {
 public:
  struct Data {
    size_t current_load{0};
    size_t overload_events_count{0};
    size_t no_overload_events_count{0};
    std::chrono::steady_clock::time_point tp;

    double GetLoadPercent() const;
  };

  virtual ~Sensor() = default;

  /// @brief Fetch current counters
  /// @note Can be called both from coroutine and non-coroutine context
  virtual Data FetchCurrent() = 0;
};

}  // namespace congestion_control

USERVER_NAMESPACE_END
