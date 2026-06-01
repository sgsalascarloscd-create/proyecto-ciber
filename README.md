# incident-backend

A backend server built with the [Drogon](https://github.com/drogonframework/drogon) C++ web framework.

> **University Assignment** — This project is developed for academic purposes as part of a university course.

[![C++20](https://img.shields.io/badge/C++-20-00599C?style=flat-square&logo=c%2B%2B)](https://isocpp.org)
[![Drogon](https://img.shields.io/badge/Drogon-1.9.13-8A2BE2?style=flat-square)](https://github.com/drogonframework/drogon)
[![CMake](https://img.shields.io/badge/CMake-%3E%3D3.22-064F8C?style=flat-square&logo=cmake)](https://cmake.org)
[![License](https://img.shields.io/badge/License-MIT-yellow?style=flat-square)](LICENSE)

[Overview](#overview) • [Tech Stack](#tech-stack) • [Prerequisites](#prerequisites) • [Getting Started](#getting-started) • [Configuration](#configuration) • [Project Structure](#project-structure)

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
