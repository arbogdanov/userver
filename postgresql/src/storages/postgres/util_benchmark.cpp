#include <storages/postgres/util_benchmark.hpp>

#include <userver/engine/task/task.hpp>

#include <storages/postgres/default_command_controls.hpp>
#include <storages/postgres/detail/connection.hpp>
#include <userver/storages/postgres/dsn.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::bench {

namespace {

Dsn GetDsnFromEnv() {
  auto* conn_list_env = std::getenv(kPostgresDsn);
  if (!conn_list_env) {
    return Dsn{{}};
  }
  auto by_host = SplitByHost(Dsn{conn_list_env});
  if (by_host.empty()) {
    return Dsn{{}};
  }
  return by_host[0];
}

}  // namespace

PgConnection::PgConnection() = default;

PgConnection::~PgConnection() = default;

void PgConnection::SetUp(benchmark::State&) {
  auto dsn = GetDsnFromEnv();
  if (!dsn.GetUnderlying().empty()) {
    engine::RunStandalone([this, dsn] {
      conn_ = detail::Connection::Connect(
          dsn, nullptr, GetTaskProcessor(), kConnectionId,
          {ConnectionSettings::kCachePreparedStatements},
          DefaultCommandControls(kBenchCmdCtl, {}, {}), {}, {});
    });
  }
}

void PgConnection::TearDown(benchmark::State&) {
  if (IsConnectionValid()) {
    engine::RunStandalone([this] { conn_->Close(); });
  }
}

bool PgConnection::IsConnectionValid() const {
  return conn_ && conn_->IsConnected();
}

engine::TaskProcessor& PgConnection::GetTaskProcessor() {
  return engine::current_task::GetTaskProcessor();
}

}  // namespace storages::postgres::bench

USERVER_NAMESPACE_END
