#pragma once

#include "request_data_impl.hpp"
#include "transaction_impl.hpp"

USERVER_NAMESPACE_BEGIN

namespace storages {
namespace redis {

class RequestExecDataImpl final : public RequestDataImplBase,
                                  public RequestDataBase<ReplyData, void> {
 public:
  RequestExecDataImpl(
      USERVER_NAMESPACE::redis::Request&& request,
      std::vector<TransactionImpl::ResultPromise>&& result_promises);

  void Wait() override;

  void Get(const std::string& request_description) override;

  ReplyPtr GetRaw() override { return GetReply(); }

 private:
  std::vector<TransactionImpl::ResultPromise> result_promises_;
};

}  // namespace redis
}  // namespace storages

USERVER_NAMESPACE_END
