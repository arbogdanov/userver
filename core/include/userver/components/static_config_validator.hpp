#pragma once

#include <userver/components/component_config.hpp>
#include <userver/yaml_config/impl/validate_static_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace components::impl {

template <typename T>
using HasGetStaticConfigSchema = decltype(T::GetStaticConfigSchema);

template <typename T>
inline constexpr bool kHasGetStaticConfigSchema =
    meta::kIsDetected<HasGetStaticConfigSchema, T>;

template <typename Component>
void ValidateStaticConfig(const components::ComponentConfig& static_config) {
  static_assert(components::kHasValidate<Component>,
                "kHasValidate must be specified for component");
  static_assert(impl::kHasGetStaticConfigSchema<Component>,
                "Component must specify GetStaticConfigSchema()");

  yaml_config::Schema schema = Component::GetStaticConfigSchema();

  yaml_config::impl::Validate(static_config, schema);
}

template <typename Component>
void TryValidateStaticConfig(const components::ComponentConfig& static_config) {
  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (components::kHasValidate<Component>) {
    ValidateStaticConfig<Component>(static_config);
  }
}

}  // namespace components::impl

USERVER_NAMESPACE_END
