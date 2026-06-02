#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <functional>

class SwaggerController {
public:
  void serveIndex(const drogon::HttpRequestPtr &req,
                  std::function<void(const drogon::HttpResponsePtr &)> &&callback);
  void serveSpec(const drogon::HttpRequestPtr &req,
                 std::function<void(const drogon::HttpResponsePtr &)> &&callback);
};
