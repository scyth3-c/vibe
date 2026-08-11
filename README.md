
# Vibe 🍃

**User-friendly and compact C++ Web Framework**

<div style=" display:flex; justify-content: center">
    <img src="https://drive.google.com/thumbnail?id=1YR1hFh0S9FR4MdWKz05PXlbWp9t1nsAr&sz=w-h" alt="vibe image">
    
</div>

## Table of Contents
1. [Installation](#installation)
    - [cmake](#cmake)
    - [docker](#docker)
    - [npm](#npm)
2. [Usage](#usage)
3. [Compile](#compile)
4. [Server configuration](#server-configuration)
5. [Render security](#render-security)
6. [Examples](#examples)
7. [Support](#support)
8. [Testing](#testing)
9. [Contribution](#contribution)
10. [License](#license)



## Installation

To install Vibe

### cmake

<img alt="CMake" src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white"/>

```shell
$ git clone https://github.com/vibecc/vibe.git
$ cd Vibe
$ cmake .
$ cmake --build .
$ make install
```

### npx

<img alt="NodeJS" src="https://img.shields.io/badge/node.js-%2343853D.svg?style=for-the-badge&logo=node-dot-js&logoColor=white"/>

#### ready to use 
```shell
$ npx create-vibe-static
```

### docker

[![Docker](https://img.shields.io/badge/Docker-2496ED?logo=docker&logoColor=fff)](#)

```shell
docker pull vibecc/vibe
```


## Usage

To use Vibe in your project, include the header files and link the static library in your C++ compiler.

```cpp
#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {
        web.send("Hello World");
    }});


    router.listen();
}
```

## Compile
#### compile your project
```bash
$ g++ -std=c++20  main.cpp -o server -L. -lvibe
```

## Server configuration

Every knob of the request/response pipeline lives in `vibe::Config`
(`include/vibe/config.hpp`). Pass it whole with `router.configure({...})`
(defaults preserve the legacy behavior):

```cpp
router.configure({
    // network
    .backlog           = SOMAXCONN, // pending connections queue of listen()

    // request reading
    .read_timeout      = std::chrono::seconds{30}, // inactivity between chunks
    .write_timeout     = std::chrono::seconds{10}, // inactivity while responding
    .max_request_size  = 16UL * 1024UL * 1024UL,   // bigger => 413 Payload Too Large
    .read_chunk        = 32UL * 1024UL,            // bytes read per recv() call

    // concurrency / epoll
    .threads           = 4,    // worker threads; 0 = auto (hardware_concurrency)
    .max_events        = 1024, // epoll event batch size
    .max_queue_size    = 512,  // queued tasks before backpressure; 0 = auto
    .epoll_timeout     = std::chrono::milliseconds{1000},
});
```

Or use the chainable setters:

```cpp
router.setThreads(4)
      .setMaxRequestSize(16UL * 1024UL * 1024UL)
      .setReadTimeout(std::chrono::seconds{30});
// setWriteTimeout, setReadChunkSize, setMaxEvents,
// setMaxQueueSize, setBacklog, setBufferSize, setPort
```

The active configuration is readable at runtime with `router.config()`.
A full annotated example lives in [`examples/configuration`](examples/configuration/main.cpp).

## Render security

The file-rendering methods (`readFile`, `file`, `readFileX`, `compose`,
`render`) are hardened through `Config::render`:

```cpp
router.configure({
    .render = {
        .root             = "public/", // jail: no path escapes this directory
        .max_file_bytes   = 32UL * 1024 * 1024,
        .allow_readfilex  = true,      // C++ templates ($ ... $)
        .compile_timeout  = std::chrono::milliseconds{15000},
        .run_timeout      = std::chrono::milliseconds{5000},
        .run_memory_bytes = 256UL * 1024 * 1024,
        .max_output_bytes = 8UL * 1024 * 1024,
    },
});
```

- All readers serve **regular files only** (no FIFOs/devices), cap the size
  in memory, and never leak internal errors to the client.
- `compose()` module names (`#[name];`) are restricted to bare file names,
  so `#[../../etc/passwd];` is rejected.
- `readFileX` compiles and executes inside a sandbox: private `mkdtemp`
  workspace, scrubbed environment, no inherited file descriptors, rlimits
  (CPU/memory/output/processes) and wall-clock timeouts enforced with
  `SIGKILL`. Compiled binaries are cached (SHA-256 of the source) under a
  private per-user directory, so steady-state requests skip `g++`.
- Set `.root` in production: without it there is no jail (legacy behavior).

### readFileX toolchain

How the embedded C++ of a `readFileX` template is compiled is fully
configurable through `Config::render::cpp`:

```cpp
router.configure({
    .render = {
        .cpp = {
            .compiler = "g++-12",       // absolute path or bare name; "" = auto-detect
            .standard = "c++20",        // passed as -std=<standard>
            .optimize = "-O2",          // "" = no optimization flag
            .hardening         = true,  // stack protector, _FORTIFY_SOURCE, strip
            .suppress_warnings  = true,  // -w
            .flags    = {"-I", "templates/includes", "-lm"}, // appended last
            .memory_bytes    = 2UL * 1024 * 1024 * 1024, // compiler RLIMIT_AS
            .file_size_bytes = 128UL * 1024 * 1024,        // compiler RLIMIT_FSIZE
        },
    },
});

// or with the fluent setter:
router.setCppToolchain({ .standard = "c++20" });
```

The toolchain is part of the binary cache key: rebuilding the same
template with different flags never serves a stale binary.


## Examples

In the [`examples/`](examples/README.md) folder you'll find self-contained
servers for the different use cases:

- **basics**: [hello-world](examples/hello-world/main.cpp),
  [types-routes](examples/types-routes/main.cpp) (all HTTP methods),
  [callbacks](examples/callbacks/main.cpp)
- **requests**: [parameters-methods](examples/parameters-methods/main.cpp)
  (typed `as<T>()`, fallbacks), [request-body](examples/request-body/main.cpp)
  (raw JSON/text bodies), [upload](examples/upload/main.cpp) (multipart files),
  [headers](examples/headers/main.cpp)
- **responses**: [simple-json](examples/simple-json/main.cpp),
  [files](examples/files/main.cpp) (auto-MIME with `file()`),
  [file-template](examples/file-template/main.cpp) (`compose`),
  [data-template](examples/data-template/main.cpp) (`render`)
- **server**: [configuration](examples/configuration/main.cpp)
  (`router.configure`, thread pool, timeouts),
  [route-cooling](examples/route-cooling/main.cpp) (`web.guard`),
  [graceful-shutdown](examples/graceful-shutdown/main.cpp),
  [middlewares](examples/middlewares/main.cpp),
  [router](examples/router/) (route separation with `Route_t` + `use`)

## Support

<img alt="Linux" src="https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black">

## Testing

cmake shell basic commands
```shell
cmake -DTESTING=ON -S. -B build 
cmake --build build/
cd build/
ctest
```

with NPM

```shell
npm run build
npm run test
```

### Debug

for debug
```shell
npm run dev:run
```

and modify the file tests/debug.cpp


## Contribution

Contributions are welcome! If you want to contribute to Vibe, please follow these guidelines:
- Fork the repository.
- Create a branch for your new feature (`git checkout -b feature/new-feature`).
- Make your changes and commit meaningful messages.
- Push your branch (`git push origin feature/new-feature`).
- Create a pull request.

## License

This project is licensed under the [MIT License](LICENSE).
