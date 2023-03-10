#pragma once

USERVER_NAMESPACE_BEGIN

namespace yaml_config {

struct Schema;

}  // namespace yaml_config

namespace components {

/// State of the component
enum class ComponentHealth {
  /// component is alive and fine
  kOk,
  /// component in fallback state, but the service keeps working
  kFallback,
  /// component is sick, service can not work without it
  kFatal,
};

namespace impl {

/// Don't use it for application components, use LoggableComponentBase instead
class ComponentBase {
 public:
  ComponentBase() = default;

  ComponentBase(ComponentBase&&) = delete;

  ComponentBase(const ComponentBase&) = delete;

  virtual ~ComponentBase();

  virtual ComponentHealth GetComponentHealth() const {
    return ComponentHealth::kOk;
  }

  virtual void OnLoadingCancelled() {}

  virtual void OnAllComponentsLoaded() {}

  virtual void OnAllComponentsAreStopping() {}

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace impl

/// Specialize it for typename Component to validate static config against
/// schema from Component::GetStaticConfigSchema
///
/// @see @ref static-configs-validation "Static configs validation"
template <typename Component>
inline constexpr bool kHasValidate = false;

}  // namespace components

USERVER_NAMESPACE_END
