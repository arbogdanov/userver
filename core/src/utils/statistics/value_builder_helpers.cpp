#include <utils/statistics/value_builder_helpers.hpp>

#include <boost/algorithm/string/join.hpp>

#include <userver/formats/common/utils.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils::statistics {

namespace {

void CheckedMerge(formats::json::ValueBuilder& original,
                  formats::json::ValueBuilder&& patch) {
  if (patch.IsObject() && original.IsObject()) {
    for (const auto& [elem_key, elem_value] : Items(patch)) {
      auto next_origin = original[elem_key];
      CheckedMerge(next_origin, std::move(elem_value));
    }
  } else {
    UASSERT_MSG(
        original.IsNull(),
        fmt::format(
            "Conflicting metrics at '{}'",
            formats::json::ValueBuilder(patch).ExtractValue().GetPath()));
    original = std::move(patch);
  }
}

}  // namespace

void SetSubField(formats::json::ValueBuilder& object,
                 std::vector<std::string>&& path,
                 formats::json::ValueBuilder&& value) {
  if (path.empty()) {
    CheckedMerge(object, std::move(value));
  } else {
    auto child = formats::common::GetAtPath(object, std::move(path));
    CheckedMerge(child, std::move(value));
  }
}

std::string JoinPath(const std::vector<std::string>& path) {
  return boost::algorithm::join(path, ".");
}

std::optional<std::string> FindNonNumberMetricPath(
    const formats::json::Value& json) {
  for (const auto& [name, value] : Items(json)) {
    if (name == "$meta") {
      continue;
    }
    if (value.IsObject()) {
      auto path = FindNonNumberMetricPath(value);
      if (path.has_value()) {
        return path;
      }
    } else if (value.IsInt() || value.IsInt64() || value.IsUInt64() ||
               value.IsDouble()) {
      continue;
    } else {
      return value.GetPath();
    }
  }

  return std::nullopt;
}

bool AreAllMetricsNumbers(const formats::json::Value& json) {
  const auto path = FindNonNumberMetricPath(json);
  UASSERT_MSG(!path.has_value(),
              "Some metrics are not numbers, path: " + path.value());
  return true;
}

}  // namespace utils::statistics

USERVER_NAMESPACE_END
