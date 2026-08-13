# syntax=docker/dockerfile:1

# Stage 1: compila el servidor vermell :)
FROM debian:bookworm AS builder

# Dependencias solo de compilacion (no van al runtime)
RUN --mount=type=cache,target=/var/cache/apt \
    --mount=type=cache,target=/var/lib/apt/lists \
    apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Solo el codigo necesario para compilar (ver .dockerignore)
COPY CMakeLists.txt README.md ./
COPY sources ./sources
COPY include ./include
COPY tests ./tests

# Build out-of-source y en paralelo
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --parallel

# Stage 2: imagen runtime minima y endurecida
FROM debian:bookworm-slim

LABEL org.opencontainers.image.title="vermell" \
      org.opencontainers.image.description="Vermell web server (C++20)" \
      org.opencontainers.image.vendor="vermell" \
      org.opencontainers.image.version="1.0.0"

# Usuario no-root para ejecutar el servidor
RUN groupadd --system --gid 1000 app \
    && useradd --system --uid 1000 --gid app --create-home --home-dir /app app

WORKDIR /app

COPY --from=builder /src/build/vermellx /app/vermellx
COPY --from=builder /src/tests/cpp.html /app/cpp.html

USER app

EXPOSE 8080

STOPSIGNAL SIGTERM

CMD ["/app/vermellx"]