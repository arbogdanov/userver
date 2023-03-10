#include "thread.hpp"

#include <chrono>
#include <csignal>
#include <stdexcept>

#include <userver/compiler/demangle.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/thread_name.hpp>
#include <utils/check_syscall.hpp>

#include "child_process_map.hpp"

USERVER_NAMESPACE_BEGIN

namespace engine::ev {
namespace {

const size_t kInitFuncQueueCapacity = 64;

std::atomic_flag& GetEvDefaultLoopFlag() {
  static std::atomic_flag ev_default_loop_flag ATOMIC_FLAG_INIT;
  return ev_default_loop_flag;
}

void AcquireEvDefaultLoop(const std::string& thread_name) {
  auto& ev_default_loop_flag = GetEvDefaultLoopFlag();
  if (ev_default_loop_flag.test_and_set())
    throw std::runtime_error(
        "Trying to use more than one ev_default_loop, thread_name=" +
        thread_name);
  LOG_DEBUG() << "Acquire ev_default_loop for thread_name=" << thread_name;
}

void ReleaseEvDefaultLoop() {
  auto& ev_default_loop_flag = GetEvDefaultLoopFlag();
  LOG_DEBUG() << "Release ev_default_loop";
  ev_default_loop_flag.clear();
}

}  // namespace

IntrusiveRefcountedBase::~IntrusiveRefcountedBase() = default;

Thread::Thread(const std::string& thread_name) : Thread(thread_name, false) {}

Thread::Thread(const std::string& thread_name, UseDefaultEvLoop)
    : Thread(thread_name, true) {}

Thread::Thread(const std::string& thread_name, bool use_ev_default_loop)
    : use_ev_default_loop_(use_ev_default_loop),
      // NOLINTNEXTLINE(clang-analyzer-core.uninitialized.Assign)
      func_queue_(kInitFuncQueueCapacity),
      loop_(nullptr),
      lock_(loop_mutex_, std::defer_lock),
      is_running_(false) {
  if (use_ev_default_loop_) AcquireEvDefaultLoop(thread_name);
  Start(thread_name);
}

Thread::~Thread() {
  StopEventLoop();
  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  if (use_ev_default_loop_) ReleaseEvDefaultLoop();
  UASSERT(loop_ == nullptr);
}

void Thread::AsyncStartUnsafe(ev_async& w) { ev_async_start(GetEvLoop(), &w); }

void Thread::AsyncStart(ev_async& w) {
  SafeEvCall([this, &w]() { AsyncStartUnsafe(w); });
}

void Thread::AsyncStopUnsafe(ev_async& w) { ev_async_stop(GetEvLoop(), &w); }

void Thread::AsyncStop(ev_async& w) {
  SafeEvCall([this, &w]() { AsyncStopUnsafe(w); });
}

void Thread::TimerStartUnsafe(ev_timer& w) {
  ev_now_update(GetEvLoop());
  ev_timer_start(GetEvLoop(), &w);
}

void Thread::TimerStart(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerStartUnsafe(w); });
}

void Thread::TimerAgainUnsafe(ev_timer& w) {
  ev_now_update(GetEvLoop());
  ev_timer_again(GetEvLoop(), &w);
}

void Thread::TimerAgain(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerAgainUnsafe(w); });
}

void Thread::TimerStopUnsafe(ev_timer& w) { ev_timer_stop(GetEvLoop(), &w); }

void Thread::TimerStop(ev_timer& w) {
  SafeEvCall([this, &w]() { TimerStopUnsafe(w); });
}

void Thread::IoStartUnsafe(ev_io& w) { ev_io_start(GetEvLoop(), &w); }

void Thread::IoStart(ev_io& w) {
  SafeEvCall([this, &w]() { IoStartUnsafe(w); });
}

void Thread::IoStopUnsafe(ev_io& w) { ev_io_stop(GetEvLoop(), &w); }

void Thread::IoStop(ev_io& w) {
  SafeEvCall([this, &w]() { IoStopUnsafe(w); });
}

void Thread::IdleStartUnsafe(ev_idle& w) { ev_idle_start(GetEvLoop(), &w); }

void Thread::IdleStart(ev_idle& w) {
  SafeEvCall([this, &w]() { IdleStartUnsafe(w); });
}

void Thread::IdleStopUnsafe(ev_idle& w) { ev_idle_stop(GetEvLoop(), &w); }

void Thread::IdleStop(ev_idle& w) {
  SafeEvCall([this, &w]() { IdleStopUnsafe(w); });
}

void Thread::RunInEvLoopAsync(
    OnRefcountedPayload* func,
    boost::intrusive_ptr<IntrusiveRefcountedBase>&& data) {
  UASSERT(func);
  UASSERT(data);

  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  if (IsInEvThread()) {
    func(*data);
    return;
  }
  if (!func_queue_.push({func, data.detach()})) {
    LOG_ERROR() << "can't push func to queue";
    throw std::runtime_error("can't push func to queue");
  }
  ev_async_send(loop_, &watch_update_);
}

bool Thread::IsInEvThread() const {
  return (std::this_thread::get_id() == thread_.get_id());
}

template <typename Func>
void Thread::SafeEvCall(const Func& func) {
  if (IsInEvThread()) {
    func();
    return;
  }

  {
    std::lock_guard<std::mutex> lock(loop_mutex_);
    func();
  }
  ev_async_send(loop_, &watch_update_);
}

void Thread::Start(const std::string& name) {
  loop_ = use_ev_default_loop_ ? ev_default_loop(EVFLAG_AUTO)
                               : ev_loop_new(EVFLAG_AUTO);
  UASSERT(loop_);
  ev_set_userdata(loop_, this);
  ev_set_loop_release_cb(loop_, Release, Acquire);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_update_, UpdateLoopWatcher);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_set_priority(&watch_update_, 1);
  ev_async_start(loop_, &watch_update_);

  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_async_init(&watch_break_, BreakLoopWatcher);
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
  ev_set_priority(&watch_break_, EV_MAXPRI);
  ev_async_start(loop_, &watch_break_);

  if (use_ev_default_loop_) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
    ev_child_init(&watch_child_, ChildWatcher, 0, 0);
    ev_child_start(loop_, &watch_child_);
  }

  is_running_ = true;
  thread_ = std::thread([this, name] {
    utils::SetCurrentThreadName(name);
    RunEvLoop();
  });
}

void Thread::StopEventLoop() {
  ev_async_send(loop_, &watch_break_);
  if (thread_.joinable()) thread_.join();
  if (!use_ev_default_loop_) ev_loop_destroy(loop_);
  loop_ = nullptr;
}

void Thread::RunEvLoop() {
  while (is_running_) {
    AcquireImpl();
    ev_run(loop_, EVRUN_ONCE);
    ReleaseImpl();
  }

  ev_async_stop(loop_, &watch_update_);
  ev_async_stop(loop_, &watch_break_);
  if (use_ev_default_loop_) ev_child_stop(loop_, &watch_child_);
}

void Thread::UpdateLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->UpdateLoopWatcherImpl();
}

void Thread::UpdateLoopWatcherImpl() {
  LOG_TRACE() << "Thread::UpdateLoopWatcherImpl() "
                 "func_queue_.empty()="
              << func_queue_.empty();

  QueueData queue_element{};
  while (func_queue_.pop(queue_element)) {
    LOG_TRACE() << "Thread::UpdateLoopWatcherImpl(), "
                << compiler::GetTypeName(typeid(*queue_element.data));
    boost::intrusive_ptr data(queue_element.data, /*add_ref = */ false);
    try {
      queue_element.func(*data);
    } catch (const std::exception& ex) {
      LOG_WARNING() << "exception in async thread func: " << ex;
    }
  }
  LOG_TRACE() << "exit";
}

void Thread::BreakLoopWatcher(struct ev_loop* loop, ev_async*, int) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  // boost.lockfree pointer magic (FP?)
  // NOLINTNEXTLINE(clang-analyzer-core.UndefinedBinaryOperatorResult)
  ev_thread->BreakLoopWatcherImpl();
}

void Thread::BreakLoopWatcherImpl() {
  is_running_ = false;
  UpdateLoopWatcherImpl();
  ev_break(loop_, EVBREAK_ALL);
}

void Thread::ChildWatcher(struct ev_loop*, ev_child* w, int) noexcept {
  try {
    ChildWatcherImpl(w);
  } catch (const std::exception& ex) {
    LOG_ERROR() << "Exception in ChildWatcherImpl(): " << ex;
  }
}

void Thread::ChildWatcherImpl(ev_child* w) {
  auto child_process_info = ChildProcessMapGetOptional(w->rpid);
  UASSERT(child_process_info);
  if (!child_process_info) {
    LOG_ERROR()
        << "Got signal for thread with pid=" << w->rpid
        << ", status=" << w->rstatus
        << ", but thread with this pid was not found in child_process_map";
    return;
  }

  auto process_status = subprocess::ChildProcessStatus{
      w->rstatus,
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now() - child_process_info->start_time)};
  if (process_status.IsExited() || process_status.IsSignaled()) {
    LOG_INFO() << "Child process with pid=" << w->rpid << " was "
               << (process_status.IsExited() ? "exited normally"
                                             : "terminated by a signal");
    child_process_info->status_promise.set_value(std::move(process_status));
    ChildProcessMapErase(w->rpid);
  } else {
    if (WIFSTOPPED(w->rstatus)) {
      LOG_WARNING() << "Child process with pid=" << w->rpid
                    << " was stopped with signal=" << WSTOPSIG(w->rstatus);
    } else {
      bool continued = WIFCONTINUED(w->rstatus);
      if (continued) {
        LOG_WARNING() << "Child process with pid=" << w->rpid << " was resumed";
      } else {
        LOG_WARNING()
            << "Child process with pid=" << w->rpid
            << " was notified in ChildWatcher with unknown reason (w->rstatus="
            << w->rstatus << ')';
      }
    }
  }
}

void Thread::Acquire(struct ev_loop* loop) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->AcquireImpl();
}

void Thread::Release(struct ev_loop* loop) noexcept {
  auto* ev_thread = static_cast<Thread*>(ev_userdata(loop));
  UASSERT(ev_thread != nullptr);
  ev_thread->ReleaseImpl();
}

void Thread::AcquireImpl() noexcept { lock_.lock(); }
void Thread::ReleaseImpl() noexcept { lock_.unlock(); }

}  // namespace engine::ev

USERVER_NAMESPACE_END
