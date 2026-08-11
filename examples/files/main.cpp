#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    // readFile with an explicit Content-Type
    router.get("/",{[&](Query &web) {

        web.readFile("./test.json", "application/json");
    }});


    // file() detects the Content-Type from the extension on its own
    // (readFile(path) without a type does the same)
    router.get("/auto",{[&](Query &web) {

        web.file("./test.json"); // served as application/json
    }});


    // readFileX: C++ template, compiled and executed inside a sandbox
    // (turn it off with .render = { .allow_readfilex = false })
    router.get("/cpp",{[&](Query &web) {

        web.readFileX("cpp.html", "text/html");
    }});


    // How the embedded C++ is compiled is fully configurable from the
    // router (compiler, standard, flags, hardening, limits):
    //
    //   router.configure({ .render = { .cpp = {
    //       .compiler = "g++-12",
    //       .standard = "c++20",
    //       .flags    = {"-I", "templates/includes"},
    //   }}});
    //
    //   // or: router.setCppToolchain({ .standard = "c++20" });


    // big-file route for stress testing; create your own stress.json,
    // it is not shipped with the repository
    router.get("/stress",{[&](Query &web) {

        web.readFile("./stress.json", "application/json");
    }});


    // In production jail every rendered path under a root directory:
    //
    //   router.configure({ .render = { .root = "public/" } });

    router.listen();
}
