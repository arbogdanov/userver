#include <userver/utest/using_namespace_userver.hpp>

#include <chrono>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <samples/greeter_service.usrv.pb.hpp>

namespace samples {

/// [gRPC sample - client]
// A user-defined wrapper around api::GreeterServiceClient that provides
// a simplified interface.
class GreeterClient final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-client";

  GreeterClient(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        // ClientFactory is used to create gRPC clients
        client_factory_(
            context.FindComponent<ugrpc::client::ClientFactoryComponent>()
                .GetFactory()),
        // The client needs a fixed endpoint
        client_(client_factory_.MakeClient<api::GreeterServiceClient>(
            config["endpoint"].As<std::string>())) {}

  std::string SayHello(std::string name);

 private:
  ugrpc::client::ClientFactory& client_factory_;
  api::GreeterServiceClient client_;
};
/// [gRPC sample - client]

/// [gRPC sample - client RPC handling]
std::string GreeterClient::SayHello(std::string name) {
  api::GreetingRequest request;
  request.set_name(std::move(name));

  // Deadline must be set manually for each RPC
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(std::chrono::seconds{20}));

  // Initiate the RPC. No actual actions have been taken thus far besides
  // preparing to send the request.
  auto stream = client_.SayHello(request, std::move(context));

  // Complete the unary RPC by sending the request and receiving the response.
  // The client should call `Finish` (in case of single response) or `Read`
  // until `false` (in case of response stream), otherwise the RPC will be
  // cancelled.
  api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}
/// [gRPC sample - client RPC handling]

/// [gRPC sample - server RPC handling]
// An implementation of GreeterService from the proto schema
class GreeterService final : public api::GreeterServiceBase {
 public:
  explicit GreeterService(std::string greeting_prefix)
      : prefix_(std::move(greeting_prefix)) {}

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) override {
    // Authentication checking could have gone here. For this example, we trust
    // the world.

    api::GreetingResponse response;
    response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

    // Complete the RPC by sending the response. The service should complete
    // each request by calling `Finish` or `FinishWithError`, otherwise the
    // client will receive an Internal Error (500) response.
    call.Finish(response);
  }

 private:
  std::string prefix_;
};
/// [gRPC sample - server RPC handling]

/// [gRPC sample - service]
class GreeterServiceComponent final
    : public ugrpc::server::ServiceComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
      : ugrpc::server::ServiceComponentBase(config, context),
        // Configuration and dependency injection for the gRPC service
        // implementation happens here.
        service_(config["greeting-prefix"].As<std::string>()) {
    // The ServiceComponentBase-derived component must provide a service
    // interface implementation here.
    RegisterService(service_);
  }

 private:
  GreeterService service_;
};
/// [gRPC sample - service]

// Our Python tests use HTTP for all the samples, so we add an HTTP handler,
// through which we test both the client side and the server side.
class GreeterHttpHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "greeter-http-handler";

  GreeterHttpHandler(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        grpc_greeter_client_(context.FindComponent<GreeterClient>()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    return grpc_greeter_client_.SayHello(request.RequestBody());
  }

 private:
  GreeterClient& grpc_greeter_client_;
};

}  // namespace samples

/// [gRPC sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      /// [gRPC sample - ugrpc registration]
      components::MinimalServerComponentList()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<ugrpc::server::ServerComponent>()
          /// [gRPC sample - ugrpc registration]
          .Append<samples::GreeterClient>()
          .Append<samples::GreeterServiceComponent>()
          .Append<samples::GreeterHttpHandler>();
  return utils::DaemonMain(argc, argv, component_list);
}
/// [gRPC service sample - main]
