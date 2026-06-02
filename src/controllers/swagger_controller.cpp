#include "swagger_controller.h"
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

namespace {

const std::string kSwaggerHtml = R"(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <title>Incident Backend API - Swagger UI</title>
    <link rel="stylesheet" type="text/css" href="https://unpkg.com/swagger-ui-dist@5/swagger-ui.css" />
  </head>
  <body>
    <div id="swagger-ui"></div>
    <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-bundle.js" charset="UTF-8"></script>
    <script src="https://unpkg.com/swagger-ui-dist@5/swagger-ui-standalone-preset.js" charset="UTF-8"></script>
    <script>
      window.onload = function () {
        var ui = SwaggerUIBundle({
          url: "/swagger/openapi.json",
          dom_id: "#swagger-ui",
          deepLinking: true,
          presets: [
            SwaggerUIBundle.presets.apis,
            SwaggerUIStandalonePreset,
          ],
          plugins: [SwaggerUIBundle.plugins.DownloadUrl],
          layout: "StandaloneLayout",
        });
        window.ui = ui;
      };
    </script>
  </body>
</html>)";

const std::string kOpenApiSpec = R"({
  "openapi": "3.0.3",
  "info": {
    "title": "Incident Backend API",
    "description": "RESTful API for managing incidents built with Drogon C++ framework.",
    "version": "1.0.0"
  },
  "paths": {
    "/api/v1/incidents": {
      "get": {
        "summary": "List all incidents",
        "tags": ["Incidents"],
        "responses": {
          "200": {
            "description": "A list of incidents",
            "content": {
              "application/json": {
                "schema": {
                  "type": "array",
                  "items": {
                    "$ref": "#/components/schemas/Incident"
                  }
                }
              }
            }
          }
        }
      },
      "post": {
        "summary": "Create a new incident",
        "tags": ["Incidents"],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/CreateIncident"
              }
            }
          }
        },
        "responses": {
          "201": {
            "description": "Incident created"
          },
          "400": {
            "description": "Bad request"
          }
        }
      }
    },
    "/api/v1/incidents/{id}": {
      "get": {
        "summary": "Get an incident by ID",
        "tags": ["Incidents"],
        "parameters": [
          {
            "name": "id",
            "in": "path",
            "required": true,
            "schema": {
              "type": "integer"
            }
          }
        ],
        "responses": {
          "200": {
            "description": "Incident details",
            "content": {
              "application/json": {
                "schema": {
                  "$ref": "#/components/schemas/Incident"
                }
              }
            }
          },
          "404": {
            "description": "Incident not found"
          }
        }
      },
      "put": {
        "summary": "Update an incident",
        "tags": ["Incidents"],
        "parameters": [
          {
            "name": "id",
            "in": "path",
            "required": true,
            "schema": {
              "type": "integer"
            }
          }
        ],
        "requestBody": {
          "required": true,
          "content": {
            "application/json": {
              "schema": {
                "$ref": "#/components/schemas/CreateIncident"
              }
            }
          }
        },
        "responses": {
          "200": {
            "description": "Incident updated"
          },
          "400": {
            "description": "Bad request"
          },
          "404": {
            "description": "Incident not found"
          }
        }
      },
      "delete": {
        "summary": "Delete an incident",
        "tags": ["Incidents"],
        "parameters": [
          {
            "name": "id",
            "in": "path",
            "required": true,
            "schema": {
              "type": "integer"
            }
          }
        ],
        "responses": {
          "204": {
            "description": "Incident deleted"
          },
          "404": {
            "description": "Incident not found"
          }
        }
      }
    }
  },
  "components": {
    "schemas": {
      "Incident": {
        "type": "object",
        "properties": {
          "id": {
            "type": "integer"
          },
          "description": {
            "type": "string"
          },
          "actor": {
            "type": "string"
          },
          "threat": {
            "type": "string"
          },
          "responsible": {
            "type": "string"
          },
          "status": {
            "type": "string"
          },
          "priority": {
            "type": "integer"
          },
          "created_at": {
            "type": "integer"
          },
          "updated_at": {
            "type": "integer"
          }
        }
      },
      "CreateIncident": {
        "type": "object",
        "required": ["description", "actor"],
        "properties": {
          "description": {
            "type": "string"
          },
          "actor": {
            "type": "string"
          },
          "threat": {
            "type": "string"
          },
          "responsible": {
            "type": "string"
          },
          "status": {
            "type": "string"
          },
          "priority": {
            "type": "integer"
          }
        }
      }
    }
  }
})";

} // namespace

void SwaggerController::serveIndex(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k200OK);
  resp->setBody(kSwaggerHtml);
  resp->setContentTypeCode(drogon::CT_TEXT_HTML);
  callback(resp);
}

void SwaggerController::serveSpec(
    const drogon::HttpRequestPtr &req,
    std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
  auto resp = drogon::HttpResponse::newHttpResponse();
  resp->setStatusCode(drogon::k200OK);
  resp->setBody(kOpenApiSpec);
  resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
  callback(resp);
}
