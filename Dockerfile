# syntax=docker/dockerfile:1.6

ARG DEBIAN_VERSION=bookworm

# ---- Build stage ----
FROM debian:${DEBIAN_VERSION}-slim AS builder

ARG DEBIAN_VERSION

ENV DEBIAN_FRONTEND=noninteractive \
    VCPKG_ROOT=/opt/vcpkg \
    VCPKG_DEFAULT_BINARY_CACHE=/opt/vcpkg-cache \
    PATH=/opt/vcpkg:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        ca-certificates \
        cmake \
        curl \
        git \
        ninja-build \
        pkg-config \
        python3 \
        tar \
        unzip \
        zip \
    && rm -rf /var/lib/apt/lists/*

# Bootstrap vcpkg (manifest mode is used, so dependencies are pulled from vcpkg.json)
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}" \
    && "${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /src

# Copy build manifests first
COPY CMakeLists.txt CMakePresets.json vcpkg.json ./

# NOW COPY SOURCES BEFORE CONFIGURATION
COPY src ./src

# Configure & Install dependencies (vcpkg manifest mode kicks in here)
RUN --mount=type=cache,target=/opt/vcpkg-cache \
    cmake --preset ninja-debug \
        -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake

# 2. Build (Removed the /src/build cache mount)
RUN --mount=type=cache,target=/opt/vcpkg-cache \
    cmake --build --preset ninja-debug

# ---- Runtime stage ----
FROM debian:${DEBIAN_VERSION}-slim AS runtime

ARG DEBIAN_VERSION

ENV DEBIAN_FRONTEND=noninteractive

# Runtime shared libraries. Anything not present here must be copied from the builder
# (vcpkg dynamic libs) in the COPY step below.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        libbrotli1 \
        libc-ares2 \
        libjsoncpp25 \
        libsqlite3-0 \
        libssl3 \
        libuuid1 \
        zlib1g \
    && rm -rf /var/lib/apt/lists/* \
    && groupadd --system --gid 1001 app \
    && useradd  --system --uid 1001 --gid app --no-create-home --shell /usr/sbin/nologin app

# Bring in vcpkg's dynamic libraries (drogon, trantor, brotli dec, etc.) and refresh the loader cache
COPY --from=builder /src/build/ninja-debug/vcpkg_installed/x64-linux/lib/ /usr/local/lib/
RUN ldconfig

WORKDIR /app

# Application binary and example env file
COPY --from=builder /src/build/ninja-debug/incident-backend-cpp /app/incident-backend-cpp
# COPY --from=builder --chown=app:app /src/.env.example /app/.env.example

# Persistent + writable directories
RUN mkdir -p /data /app/uploads \
    && chown -R app:app /data /app/uploads

USER app

ENV PORT=8080 \
    DB_PATH=/data/backend.db \
    LOG_LEVEL=info

EXPOSE 8080

VOLUME ["/data", "/app/uploads"]

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -fsS "http://127.0.0.1:${PORT}/api/v1/incidents" >/dev/null || exit 1

ENTRYPOINT ["/app/incident-backend-cpp"]
