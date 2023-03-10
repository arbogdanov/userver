#include <userver/components/minimal_component_list.hpp>

#include <userver/components/manager_controller_component.hpp>
#include <userver/components/statistics_storage.hpp>
#include <userver/components/tracer.hpp>
#include <userver/dynamic_config/fallbacks/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/logging/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace components {

ComponentList MinimalComponentList() {
  return components::ComponentList()
      .Append<components::Logging>()
      .Append<components::Tracer>()
      .Append<components::ManagerControllerComponent>()
      .Append<components::StatisticsStorage>()
      .Append<components::DynamicConfig>()
      .Append<components::DynamicConfigFallbacks>();
}

}  // namespace components

USERVER_NAMESPACE_END
