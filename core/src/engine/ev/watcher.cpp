#include <engine/ev/watcher.hpp>

#include <fcntl.h>
#include <unistd.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {
[[maybe_unused]] bool IsFdValid(int fd) { return ::fcntl(fd, F_GETFD) != -1; }
}  // namespace

template <>
void Watcher<ev_async>::Init(void (*cb)(struct ev_loop*, ev_async*,
                                        int) noexcept) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&w_, cb);
}

template <>
void Watcher<ev_async>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  ev_async_start(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_async>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  ev_async_stop(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept,
                          int fd, int events) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_io_init(&w_, cb, fd, events);
}

template <>
void Watcher<ev_io>::Init(void (*cb)(struct ev_loop*, ev_io*, int) noexcept) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_init(&w_, cb);
}

template <>
template <>
void Watcher<ev_io>::Set(int fd, int events) {
  ev_io_set(&w_, fd, events);
}

template <>
void Watcher<ev_io>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  UASSERT_MSG(IsFdValid(w_.fd), "Invalid fd=" + std::to_string(w_.fd));
  ev_io_start(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_io>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  UASSERT_MSG(IsFdValid(w_.fd), "Invalid fd=" + std::to_string(w_.fd));
  ev_io_stop(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_timer>::Init(void (*cb)(struct ev_loop*, ev_timer*,
                                        int) noexcept,
                             ev_tstamp after, ev_tstamp repeat) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_init(&w_, cb, after, repeat);
}

template <>
template <>
void Watcher<ev_timer>::Set(ev_tstamp after, ev_tstamp repeat) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_timer_set(&w_, after, repeat);
}

template <>
void Watcher<ev_timer>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  ev_timer_start(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_timer>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  ev_timer_stop(thread_control_.GetEvLoop(), &w_);
}

template <>
template <>
void Watcher<ev_timer>::AgainImpl() {
  is_running_ = true;
  ev_timer_again(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_idle>::Init(void (*cb)(struct ev_loop*, ev_idle*,
                                       int) noexcept) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_idle_init(&w_, cb);
}

template <>
void Watcher<ev_idle>::StartImpl() {
  if (is_running_) return;
  is_running_ = true;
  ev_idle_start(thread_control_.GetEvLoop(), &w_);
}

template <>
void Watcher<ev_idle>::StopImpl() {
  if (!is_running_) return;
  is_running_ = false;
  ev_idle_stop(thread_control_.GetEvLoop(), &w_);
}

}  // namespace engine::ev

USERVER_NAMESPACE_END
