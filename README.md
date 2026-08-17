
# Vermell 🍃

**User-friendly and compact C++ Web Framework**

<div style=" display:flex; justify-content: center">
    <img src="https://drive.google.com/thumbnail?id=1YR1hFh0S9FR4MdWKz05PXlbWp9t1nsAr&sz=w-h" alt="vermell image">
    
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
6. [Process & environment](#process--environment)
7. [Examples](#examples)
8. [Support](#support)
9. [Testing](#testing)
10. [Contribution](#contribution)
11. [License](#license)



## Installation

To install Vermell

### cmake

<img alt="CMake" src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=for-the-badge&logo=cmake&logoColor=white"/>

```shell
$ git clone https://github.com/vermellcc/vermell.git
$ cd Vermell
$ cmake .
$ cmake --build .
$ make install
```

### npx

<img alt="NodeJS" src="https://img.shields.io/badge/node.js-%2343853D.svg?style=for-the-badge&logo=node-dot-js&logoColor=white"/>

#### ready to use 
```shell
$ npx create-vermell-static
```

### docker

[![Docker](https://img.shields.io/badge/Docker-2496ED?logo=docker&logoColor=fff)](#)

```shell
docker pull vermellcc/vermell
```


## Usage

To use Vermell in your project, include the header files and link the static library in your C++ compiler.

```cpp
#include <vermell/vermell.h>

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
$ g++ -std=c++20  main.cpp -o server -L. -lvermell
```

## Server configuration

Every knob of the request/response pipeline lives in `vermell::Config`
(`include/vermell/config.hpp`). Pass it whole with `router.configure({...})`
(defaults preserve the legacy behavior):

```cpp
router.configure({
    // network
    .backlog           = SOMAXCONN, // pending connections queue of listen()
    .reuse_port        = false,     // SO_REUSEPORT: OFF by default (a same-UID
                                    // process could otherwise bind the port and
                                    // intercept a share of the traffic)

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

> **Hardening defaults:** `read_chunk` is clamped to `[1, 1 MiB]`, `max_events` to
> `[1, 65536]`, `threads` to `[0, 256]` and every timeout to `[1ms, INT_MAX ms]`
> — absurd values are a memory/DoS foot-gun, not a feature. Requests to
> HTTP/1.1 (or newer) without exactly one `Host` header are rejected with 400
> (RFC 9112 §3.2, proxy desync / request-smuggling vector); HTTP/1.0 legacy
> clients keep working.

Or use the chainable setters:

```cpp
router.setThreads(4)
      .setMaxRequestSize(16UL * 1024UL * 1024UL)
      .setReadTimeout(std::chrono::seconds{30});
// setWriteTimeout, setReadChunkSize, setMaxEvents,
// setMaxQueueSize, setBacklog, setBufferSize, setPort, setReusePort
```

The active configuration is readable at runtime with `router.config()`.
A full annotated example lives in [`examples/configuration`](examples/configuration/main.cpp).

## MIME types and file rendering

Vermell detects the `Content-Type` from the final extension of a file. This
means `kevin.txt.html` is served as `text/html`, and matching is
case-insensitive. Query strings and fragments are ignored when determining the
type. Unknown extensions use `application/octet-stream`.

```cpp
web.readFile("public/data.json");       // application/json
web.readFileX("public/page.html");      // text/html
web.file("public/assets/app.js");       // application/javascript

web.send("{}", vermell::mime::json);     // reusable common MIME constants
```

An explicit type passed to `readFile` or `readFileX` always takes precedence.
The registry includes common text, data, document, image, audio, video, font,
archive and executable formats.

### readFileX: C++ templates

> **Security:** `readFileX` compiles and runs embedded C++ on the server, so
> it is **disabled by default**. Enable it with
> `router.configure({ .render = { .allow_readfilex = true } })` only when the
> template content is trusted.

A template may hold **any number** of `$ ... $` blocks. Each block runs at
its position in the page and whatever it writes to `std::cout` is spliced
right there; the markup in between is served byte-exact:

```html
<body>
    $
        for (int i = 0; i < 10; i++) {
            std::cout << "<button> soy un boton, numero: " << i << "</button>";
        }
    $

    $

    std::cout << "<button>test</button>";

    $
</body>
```

Blocks share a single `main()`, so variables declared in an earlier block are
visible in later ones. A `$` without a closing partner is treated as literal
text (a price like `$5` never breaks the page). The generated program embeds
the static markup as fully escaped string literals, so template text cannot
inject code into the compilation.

## Render security

The file-rendering methods (`readFile`, `file`, `readFileX`, `compose`,
`render`) are hardened through `Config::render`:

```cpp
router.configure({
    .render = {
        .root             = "public/", // jail: no path escapes this directory
        .max_file_bytes   = 32UL * 1024 * 1024,
        .allow_readfilex  = true,      // C++ templates ($ ... $); OFF by default
        .compile_timeout  = std::chrono::milliseconds{15000},
        .run_timeout      = std::chrono::milliseconds{5000},
        .run_memory_bytes = 256UL * 1024 * 1024,
        .max_output_bytes = 8UL * 1024 * 1024,
    },
});
```

- All readers serve **regular files only** (no FIFOs/devices, symlinks are
  rejected via `O_NOFOLLOW`), cap the size in memory, and never leak
  internal errors to the client.
- `compose()` module names (`#[name];`) are restricted to bare file names,
  so `#[../../etc/passwd];` is rejected, and the composed page is capped at
  `max_file_bytes` per pass — a module that (transitively) includes itself
  answers 413 instead of exhausting memory.
- **`readFileX` is OFF by default.** It compiles and executes embedded C++,
  so it must be enabled explicitly (`.allow_readfilex = true`) only when the
  template content is trusted. When enabled, execution is sandboxed: private
  `mkdtemp` workspace (0700/0711), scrubbed environment, no inherited file
  descriptors, rlimits (CPU/memory/output/processes/file-descriptors),
  wall-clock timeouts enforced with `SIGKILL`, and — when the server runs as
  root — the template is executed as the `nobody` user. Compiled binaries are
  cached (SHA-256 of the source) under a private per-user directory, so
  steady-state requests skip `g++` (the binary is 0755: a dynamically-linked
  ELF needs read access for `ld.so` even with execute permission).
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

## Process & environment

Node.js-style runtime information and configuration, available just by
including `vermell/vermell.h`.

`vermell::process` captures the process data once (first use):

```cpp
vermell::process.pwd        // directory containing the executable
vermell::process.cwd        // working directory it was launched from
vermell::process.exec_path  // absolute path of the executable
vermell::process.pid        // process id (also ppid, argv, hostname,
                         // username, platform, arch)
vermell::process.uptime()        // seconds since the process started
vermell::process.memory_usage()  // resident memory in bytes
vermell::process.path(".env")    // path resolved against the executable directory
```

`vermell::environment` loads the `.env` file sitting **next to the
executable** automatically, and also holds runtime "session" values.
Values from the file and `set()` take precedence over the OS
environment; every method is thread-safe.

```cpp
vermell::environment.get("TOKEN")              // .env / set(), else OS env, else ""
vermell::environment.get("TOKEN", "fallback")
vermell::environment.get_as<int>("PORT", 8080) // typed: arithmetic, bool, string
vermell::environment["TOKEN"]

vermell::environment.set("request_count", "1") // runtime session value
vermell::environment.reload()                  // re-read the .env file
vermell::environment.load("config/.env")       // or load another file
```

The `.env` syntax supports `#` comments, `export KEY=VALUE`, quoted
values and trailing comments. See
[`examples/process`](examples/process/main.cpp) and
[`examples/environment`](examples/environment/main.cpp).

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
  [router](examples/router/) (route separation with `Route_t` + `use`),
  [process](examples/process/main.cpp) (`vermell::process` runtime info),
  [environment](examples/environment/main.cpp) (`.env` + session values
  with `vermell::environment`)

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

Contributions are welcome! If you want to contribute to Vermell, please follow these guidelines:
- Fork the repository.
- Create a branch for your new feature (`git checkout -b feature/new-feature`).
- Make your changes and commit meaningful messages.
- Push your branch (`git push origin feature/new-feature`).
- Create a pull request.

## License

This project is licensed under the [MIT License](LICENSE).
