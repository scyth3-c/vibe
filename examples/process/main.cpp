#include <vermell/vermell.h>

//
// Process information (Node.js-style): the fields of vermell::process are
// captured once on first use. `using vermell::process;` enables the short
// Node-like form.
//
//   curl http://localhost:8080/
//   curl http://localhost:8080/info
//
int main() {

    using vermell::process;

    Router router;
    router.setPort(8080);


    router.get("/",{[](Query &web) {
        web.send("hello from " + process.exec_name +
                 " (pid " + std::to_string(process.pid) + ")");
    }});


    router.get("/info",{[](Query &web) {
        vermell::Json::array_t args;
        for (const auto& arg : process.argv)
            args.emplace_back(arg);

        web.json(vermell::Json::object({
            {"pwd",        process.pwd},        // directory containing the executable
            {"cwd",        process.cwd},        // directory it was launched from
            {"exec_path",  process.exec_path},  // absolute path of the executable
            {"exec_name",  process.exec_name},
            {"pid",        process.pid},
            {"ppid",       process.ppid},
            {"platform",   process.platform},
            {"arch",       process.arch},
            {"hostname",   process.hostname},
            {"username",   process.username},
            {"argv",       vermell::Json(std::move(args))},
            {"uptime",     process.uptime()},        // seconds since start
            {"memory_rss", process.memory_usage()},  // resident bytes
        }).dump());
    }});


    // paths resolved against the executable directory:
    //   process.path(".env")  -> "<pwd>/.env"
    router.get("/whoami",{[](Query &web) {
        web.send(process.username + "@" + process.hostname +
                 " running " + process.path(".env"));
    }});


    router.listen();
}
