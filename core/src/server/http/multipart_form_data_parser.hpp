#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <userver/server/http/form_data_arg.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace http {

using FormDataArgs = std::unordered_map<std::string, std::vector<FormDataArg>>;

bool IsMultipartFormDataContentType(std::string_view content_type);
bool ParseMultipartFormData(const std::string& content_type,
                            std::string_view body, FormDataArgs& form_data_args,
                            bool strict_cr_lf = false);

}  // namespace http
}  // namespace server

USERVER_NAMESPACE_END
