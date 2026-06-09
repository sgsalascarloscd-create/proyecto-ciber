# Changelog

All notable changes to this project will be documented in this file.

## [0.1.0] - 2026-06-09

### Added

- **CORS support**: Preflight handling via `registerPreRoutingAdvice`, plus custom response headers on incident endpoints
- **Frontend static site**: Single-page frontend (`frontend/index.html`) with configurable API endpoint via ALB DNS injection
- **CI/CD pipelines**: GitHub Actions workflows for CI (Docker build + Terraform validation) and deployment (ECS with OIDC auth and rolling updates)
- **Terraform S3 website**: Static website hosting bucket with public access policy, variables for index/error documents
- **Terraform bootstrap**: S3 + DynamoDB backend infrastructure for remote state management with encryption and locking
- **AWS ECS Fargate**: Complete infrastructure with VPC, ALB, ECR, ECS service, and CloudWatch logging
- **Database layer**: SQLite storage wrapper with CRUD operations and incident seeding
- **REST API**: Incident CRUD endpoints (`GET/POST/PUT/DELETE /api/v1/incidents`)
- **Swagger UI**: OpenAPI spec serving and Swagger UI at `/swagger`
- **Docker support**: Multi-stage Dockerfile with vcpkg-based dependency management and docker-compose for local dev
- **Build system**: CMake + Ninja with CMakePresets

### Changed

- **Terraform formatting**: Consistent alignment in ECS and outputs resources
- **README tables**: Standardized markdown table formatting

### Infrastructure

- `terraform/` — ECR, ECS, S3, ALB, VPC, security groups, IAM roles
- `terraform/bootstrap/` — Remote state S3 bucket + DynamoDB lock table
- `.github/workflows/` — CI (Docker build, Terraform validate) and Deploy (4-job pipeline with OIDC)
