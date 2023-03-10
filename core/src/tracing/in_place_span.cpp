#include <userver/tracing/in_place_span.hpp>

#include <utility>

#include <tracing/span_impl.hpp>
#include <userver/utils/uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {

namespace {

void SetLinkIfRoot(tracing::Span& span) {
  if (span.GetLink().empty()) {
    span.SetLink(utils::generators::GenerateUuid());
  }
}

}  // namespace

struct InPlaceSpan::Impl final {
  template <typename... Args>
  explicit Impl(Args&&... args)
      : span_impl(std::forward<Args>(args)...), span(span_impl) {}

  tracing::Span::Impl span_impl;
  tracing::Span span;
};

InPlaceSpan::InPlaceSpan(std::string&& name) : impl_(std::move(name)) {
  SetLinkIfRoot(impl_->span);
}

InPlaceSpan::InPlaceSpan(std::string&& name, std::string&& trace_id,
                         std::string&& parent_span_id)
    : impl_(std::move(name)) {
  impl_->span_impl.SetTraceId(std::move(trace_id));
  impl_->span_impl.SetParentId(std::move(parent_span_id));
  SetLinkIfRoot(impl_->span);
}

InPlaceSpan::~InPlaceSpan() = default;

tracing::Span& InPlaceSpan::Get() noexcept { return impl_->span; }

}  // namespace tracing

USERVER_NAMESPACE_END
