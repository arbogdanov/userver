#include <userver/dynamic_config/snapshot.hpp>

#include <dynamic_config/storage_data.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/rcu/rcu.hpp>

USERVER_NAMESPACE_BEGIN

namespace dynamic_config {

struct Snapshot::Impl final {
  explicit Impl(const impl::StorageData& storage)
      : data_ptr(storage.config.Read()) {}

  rcu::ReadablePtr<impl::SnapshotData> data_ptr;
};

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Snapshot::Snapshot(const Snapshot&) = default;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
Snapshot::Snapshot(Snapshot&&) noexcept = default;

Snapshot& Snapshot::operator=(const Snapshot&) = default;

Snapshot& Snapshot::operator=(Snapshot&&) noexcept = default;

Snapshot::~Snapshot() = default;

Snapshot::Snapshot(const impl::StorageData& storage) : impl_(storage) {}

const impl::SnapshotData& Snapshot::GetData() const { return *impl_->data_ptr; }

}  // namespace dynamic_config

USERVER_NAMESPACE_END
