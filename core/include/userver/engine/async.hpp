#pragma once

/// @file userver/engine/async.hpp
/// @brief TaskWithResult creation helpers

#include <userver/engine/deadline.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

template <template <typename> typename TaskType, typename Function,
          typename... Args>
[[nodiscard]] auto MakeTaskWithResult(TaskProcessor& task_processor,
                                      Task::Importance importance,
                                      Deadline deadline, Function&& f,
                                      Args&&... args) {
  auto wrapped_call_ptr = utils::impl::WrapCall(std::forward<Function>(f),
                                                std::forward<Args>(args)...);
  using ResultType = decltype(wrapped_call_ptr->Retrieve());
  return TaskType<ResultType>(task_processor, importance, deadline,
                              std::move(wrapped_call_ptr));
}

}  // namespace impl

/// Runs an asynchronous function call using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Function&& f,
                               Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f),
      std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor,
                                     Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f),
      std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using specified task
/// processor
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Deadline deadline,
                               Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kNormal, deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using specified task
/// processor
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor,
                                     Deadline deadline, Function&& f,
                                     Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kNormal, deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Function&& f, Args&&... args) {
  return AsyncNoSpan(current_task::GetTaskProcessor(),
                     std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Function&& f, Args&&... args) {
  return SharedAsyncNoSpan(current_task::GetTaskProcessor(),
                           std::forward<Function>(f),
                           std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using task processor of the
/// caller
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Deadline deadline, Function&& f,
                               Args&&... args) {
  return AsyncNoSpan(current_task::GetTaskProcessor(), deadline,
                     std::forward<Function>(f), std::forward<Args>(args)...);
}

/// Runs an asynchronous function call with deadline using task processor of the
/// caller
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Deadline deadline, Function&& f,
                                     Args&&... args) {
  return SharedAsyncNoSpan(current_task::GetTaskProcessor(), deadline,
                           std::forward<Function>(f),
                           std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(TaskProcessor& task_processor,
                                       Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<TaskWithResult>(
      task_processor, Task::Importance::kCritical, {},
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using specified task processor
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(TaskProcessor& task_processor,
                                             Function&& f, Args&&... args) {
  return impl::MakeTaskWithResult<SharedTaskWithResult>(
      task_processor, Task::Importance::kCritical, {},
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(Function&& f, Args&&... args) {
  return CriticalAsyncNoSpan(current_task::GetTaskProcessor(),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

/// @brief Runs an asynchronous function call that must not be cancelled
/// due to overload using task processor of the caller
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(Function&& f, Args&&... args) {
  return SharedCriticalAsyncNoSpan(current_task::GetTaskProcessor(),
                                   std::forward<Function>(f),
                                   std::forward<Args>(args)...);
}

}  // namespace engine

USERVER_NAMESPACE_END
