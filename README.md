
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
4. [Examples](#examples)
5. [Support](#support)
6. [Contribution](#contribution)
7. [License](#license)



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


## Examples

In the `examples/` folder, you'll find examples of how to use Vibe for different use cases, such as dynamic routes, form handling, etc.

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
