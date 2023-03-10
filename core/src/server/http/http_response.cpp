#include <userver/server/http/http_response.hpp>

#include <array>
#include <iomanip>
#include <sstream>

#include <cctz/time_zone.h>

#include <userver/engine/io/socket.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/content_type.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/userver_info.hpp>

#include "http_request_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kCrlf = "\r\n";
constexpr std::string_view kResponseHttpVersionPrefix = "HTTP/";

const http::ContentType kDefaultContentType = "text/html; charset=utf-8";

constexpr std::string_view kClose = "close";
constexpr std::string_view kKeepAlive = "keep-alive";

void CheckHeaderName(std::string_view name) {
  static constexpr auto init = []() {
    std::array<uint8_t, 256> res{};  // zero initialize
    for (int i = 0; i < 32; i++) res[i] = 1;
    for (int i = 127; i < 256; i++) res[i] = 1;
    for (char c : "()<>@,;:\\\"/[]?={} \t") res[static_cast<int>(c)] = 1;
    return res;
  };
  static constexpr auto bad_chars = init();

  for (char c : name) {
    auto code = static_cast<uint8_t>(c);
    if (bad_chars[code]) {
      throw std::runtime_error(
          std::string("invalid character in header name: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

void CheckHeaderValue(std::string_view value) {
  for (char c : value) {
    auto code = static_cast<uint8_t>(c);
    if (code < 32 || code == 127) {
      throw std::runtime_error(
          std::string("invalid character in header value: '") + c + "' (#" +
          std::to_string(code) + ")");
    }
  }
}

bool IsBodyForbiddenForStatus(server::http::HttpStatus status) {
  return status == server::http::HttpStatus::kNoContent ||
         status == server::http::HttpStatus::kNotModified ||
         (static_cast<int>(status) >= 100 && static_cast<int>(status) < 200);
}

}  // namespace

namespace server::http {

HttpResponse::HttpResponse(const HttpRequestImpl& request,
                           request::ResponseDataAccounter& data_accounter)
    : ResponseBase(data_accounter), request_(request) {}

HttpResponse::~HttpResponse() = default;

void HttpResponse::SetSendFailed(
    std::chrono::steady_clock::time_point failure_time) {
  SetStatus(HttpStatus::kClientClosedRequest);
  request::ResponseBase::SetSendFailed(failure_time);
}

void HttpResponse::SetHeader(std::string name, std::string value) {
  CheckHeaderName(name);
  CheckHeaderValue(value);
  headers_[std::move(name)] = std::move(value);
}

void HttpResponse::SetContentType(
    const USERVER_NAMESPACE::http::ContentType& type) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentType, type.ToString());
}

void HttpResponse::SetContentEncoding(std::string encoding) {
  SetHeader(USERVER_NAMESPACE::http::headers::kContentEncoding,
            std::move(encoding));
}

void HttpResponse::SetStatus(HttpStatus status) { status_ = status; }

void HttpResponse::ClearHeaders() { headers_.clear(); }

void HttpResponse::SetCookie(Cookie cookie) {
  CheckHeaderValue(cookie.Name());
  CheckHeaderValue(cookie.Value());
  auto cookie_name = cookie.Name();
  cookies_.emplace(std::move(cookie_name), std::move(cookie));
}

void HttpResponse::ClearCookies() { cookies_.clear(); }

HttpResponse::HeadersMapKeys HttpResponse::GetHeaderNames() const {
  return HttpResponse::HeadersMapKeys{headers_};
}

const std::string& HttpResponse::GetHeader(
    const std::string& header_name) const {
  return headers_.at(header_name);
}

bool HttpResponse::HasHeader(const std::string& header_name) const {
  return headers_.find(header_name) != headers_.end();
}

HttpResponse::CookiesMapKeys HttpResponse::GetCookieNames() const {
  return HttpResponse::CookiesMapKeys{cookies_};
}

const Cookie& HttpResponse::GetCookie(const std::string& cookie_name) const {
  return cookies_.at(cookie_name);
}

void HttpResponse::SendResponse(engine::io::Socket& socket) {
  const bool is_head_request = request_.GetOrigMethod() == HttpMethod::kHead;
  const bool is_body_forbidden = IsBodyForbiddenForStatus(status_);

  std::ostringstream os;
  os << kResponseHttpVersionPrefix << request_.GetHttpMajor() << '.'
     << request_.GetHttpMinor() << ' ' << static_cast<int>(status_) << ' '
     << HttpStatusString(status_) << kCrlf;

  headers_.erase(USERVER_NAMESPACE::http::headers::kContentLength);
  if (headers_.find(USERVER_NAMESPACE::http::headers::kDate) ==
      headers_.end()) {
    static const std::string kFormatString = "%a, %d %b %Y %H:%M:%S %Z";
    static const auto tz = cctz::utc_time_zone();
    const auto& time_str =
        cctz::format(kFormatString, std::chrono::system_clock::now(), tz);
    os << USERVER_NAMESPACE::http::headers::kDate << ": " << time_str << kCrlf;
  }
  if (headers_.find(USERVER_NAMESPACE::http::headers::kContentType) ==
      headers_.end())
    os << USERVER_NAMESPACE::http::headers::kContentType << ": "
       << kDefaultContentType << kCrlf;
  for (const auto& header : headers_)
    os << header.first << ": " << header.second << kCrlf;
  if (headers_.find(USERVER_NAMESPACE::http::headers::kConnection) ==
      headers_.end())
    os << USERVER_NAMESPACE::http::headers::kConnection << ": "
       << (request_.IsFinal() ? kClose : kKeepAlive) << kCrlf;
  if (!is_body_forbidden) {
    os << USERVER_NAMESPACE::http::headers::kContentLength << ": "
       << GetData().size() << kCrlf;
  }
  for (const auto& cookie : cookies_)
    os << USERVER_NAMESPACE::http::headers::kSetCookie << ": "
       << cookie.second.ToString() << kCrlf;
  os << kCrlf;

  static const auto kMinSeparateDataSize = 50000;  //  50Kb
  const auto& data = GetData();
  bool separate_data_send = data.size() > kMinSeparateDataSize;
  if (!is_body_forbidden) {
    if (!separate_data_send && !is_head_request) {
      os << data;
    }
  } else {
    separate_data_send = false;
    if (!data.empty()) {
      LOG_LIMITED_WARNING()
          << "Non-empty body provided for response with HTTP code "
          << static_cast<int>(status_)
          << " which does not allow one, it will be dropped";
    }
  }

  const auto response_data = os.str();
  auto sent_bytes =
      socket.SendAll(response_data.data(), response_data.size(), {});

  if (separate_data_send && sent_bytes == response_data.size() &&
      !is_head_request) {
    // If response is too big, copying is more expensive than +1 syscall
    sent_bytes += socket.SendAll(data.data(), data.size(), {});
  }

  SetSentTime(std::chrono::steady_clock::now());
  SetSent(sent_bytes);
}

}  // namespace server::http

USERVER_NAMESPACE_END
