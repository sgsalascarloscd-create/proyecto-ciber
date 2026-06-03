# incident-backend

A backend server built with the [Drogon](https://github.com/drogonframework/drogon) C++ web framework.

> **University Assignment** — This project is developed for academic purposes as part of a university course.

[![C++20](https://img.shields.io/badge/C++-20-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org)
[![Drogon](https://img.shields.io/badge/Drogon-1.9.13-8A2BE2?style=flat-square)](https://github.com/drogonframework/drogon)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.22-064F8C?style=flat-square&logo=cmake)](https://cmake.org)
[![Docker](https://img.shields.io/badge/Docker-ready-2496ED?style=flat-square&logo=docker)](docker-compose.yml)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)](LICENSE)

[Overview](#overview) • [Tech Stack](#tech-stack) • [Prerequisites](#prerequisites) • [Getting Started](#getting-started) • [Running with Docker](#running-with-docker) • [Configuration](#configuration) • [Project Structure](#project-structure)

---

## Overview

Incident-backend is a C++20 HTTP server application that provides a foundation for building incident management APIs. Currently in its early stages, the project sets up a Drogon web server with configurable port, logging via spdlog, and SQLite database support via sqlite-orm.

The project uses **vcpkg** for dependency management, ensuring reproducible builds with pinned dependency versions.

## Tech Stack

| Component | Technology |
|---|---|
| Language | C++20 |
| Web Framework | [Drogon](https://github.com/drogonframework/drogon) 1.9.13 |
| Logging | [spdlog](https://github.com/gabime/spdlog) |
| Database ORM | [sqlite-orm](https://github.com/fnc12/sqlite_orm) |
| JSON | [nlohmann-json](https://github.com/nlohmann/json) 3.12.0 |
| Build System | CMake ≥ 3.22 + Ninja |
| Package Manager | [vcpkg](https://github.com/microsoft/vcpkg) (manifest mode) |
| Testing | [Catch2](https://github.com/catchorg/Catch2) |

## Prerequisites

- **CMake** ≥ 3.22
- **Ninja** build system
- **vcpkg** — set the `VCPKG_ROOT` environment variable to your vcpkg installation path
- A **C++20-compatible compiler** (Apple Clang, GCC ≥ 10, or MSVC 2022+)

## Getting Started

### 1. Clone the repository

```bash
git clone <repository-url>
cd incident-backend
```

### 2. Configure with CMake presets

```bash
cmake --preset ninja-debug
```

This will automatically download and build all dependencies via vcpkg (Drogon, spdlog, sqlite-orm, nlohmann-json, Catch2, ICU).

### 3. Build the project

```bash
cmake --build --preset ninja-debug
```

The resulting binary will be located at `build/ninja-debug/incident-backend-cpp`.

### 4. Configure environment

Copy the example environment file and adjust values as needed:

```bash
cp .env.example .env
```

See the [Configuration](#configuration) section for details.

### 5. Run the server

```bash
./build/ninja-debug/incident-backend-cpp
```

The server starts on `0.0.0.0:<PORT>` (default `8080`) with 4 worker threads.

### Manual build (without presets)

```bash
cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build .
```

## Running with Docker

The project ships with a multi-stage `Dockerfile` and a `docker-compose.yml` so you can build and run the server without installing vcpkg, CMake, or a C++ toolchain on your machine. vcpkg and the build dependencies live inside the image.

### Prerequisites

- [Docker Engine](https://docs.docker.com/engine/install/) 24+
- [Docker Compose](https://docs.docker.com/compose/install/) v2 (bundled with recent Docker Desktop)

### Build and start

From the project root:

```bash
docker compose up --build
```

The first build pulls a Debian base image, clones vcpkg, compiles every dependency (Drogon, spdlog, sqlite-orm, nlohmann-json, Catch2, ICU, and their transitive system libraries), and produces the `incident-backend-cpp` binary inside the image. Subsequent builds reuse the vcpkg and build caches.

Once the server reports `Starting server on port http://127.0.0.1:8080`, the API is reachable at `http://localhost:8080`.

### What you get

- A slim runtime image based on `debian:bookworm-slim` that runs as a non-root user.
- A named volume `incident-backend-data` mounted at `/data` so the SQLite database persists across container restarts.
- A bind mount of `./uploads` for the file upload scaffold described in the project structure.
- A `HEALTHCHECK` that pings `GET /api/v1/incidents` every 30 seconds.
- BuildKit cache mounts (`/opt/vcpkg-cache` and `/src/build`) for fast rebuilds. Make sure BuildKit is enabled:

  ```bash
  # One-off enable
  DOCKER_BUILDKIT=1 docker compose build

  # Or permanently in ~/.docker/config.json
  { "features": { "buildkit": true } }
  ```

### Common commands

```bash
# Start in the background
docker compose up -d --build

# Follow logs
docker compose logs -f api

# Stop and remove the container (the named volume is kept)
docker compose down

# Stop and also wipe the database volume
docker compose down --volumes

# Open a shell inside the running container
docker compose exec api sh

# Rebuild after pulling new sources (no cache)
docker compose build --no-cache
```

### Customizing configuration

Anything in `.env` is passed to the container via the `env_file` directive. The compose file also sets `DB_PATH=/data/backend.db` and `LOG_LEVEL` explicitly so the SQLite file lives inside the persistent volume. To override the port on the host, set `PORT` in `.env` (for example `PORT=9090`) and re-run `docker compose up`.

## Configuration

The application reads configuration from a `.env` file in the project root and falls back to default values if not set.

| Variable | Default | Description |
|---|---|---|
| `PORT` | `8080` | HTTP server listening port |
| `DB_PATH` | `backend.db` | Path to the SQLite database file |
| `LOG_LEVEL` | `info` | Log level (`trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`) |

> [!NOTE]
> The `.env` file is gitignored. Use `.env.example` as a template to create your own.

## Project Structure

```
├── CMakeLists.txt          # Root build configuration
├── CMakePresets.json        # CMake presets (Ninja Debug)
├── vcpkg.json               # vcpkg manifest (dependency versions)
├── Dockerfile               # Multi-stage build (Debian + vcpkg)
├── docker-compose.yml       # Compose service for local development
├── .dockerignore            # Files excluded from the Docker build context
├── .env.example             # Example environment file
├── src/
│   ├── main.cpp             # Application entry point
│   └── config.h             # Environment configuration loader
├── tests/
│   └── CMakeLists.txt       # Test subdirectory (Catch2 ready)
└── uploads/
    └── tmp/                 # File upload storage (00-FF sharded directories)
```

### What's included

- **Environment config loader** (`src/config.h`): A custom `.env` parser that loads key-value pairs into environment variables, with fallback defaults.
- **Drogon HTTP server** (`src/main.cpp`): Listens on `0.0.0.0` with configurable port and thread count.
- **spdlog integration**: Logging configured with level control via `LOG_LEVEL`.
- **SQLite ORM**: `sqlite-orm` is linked and ready for database schema definition.
- **File upload scaffolding**: The `uploads/tmp/` directory with 256 hex-named subdirectories is pre-created for hash-based file storage.

### Coming soon

- API route handlers and controllers
- Database models and migrations
- Unit and integration tests with Catch2
- Incident management CRUD operations
- File upload handling

## Contributing

This is a university project and is not open for external contributions at this time.

## License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
