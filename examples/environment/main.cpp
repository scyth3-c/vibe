#include <iostream>
#include <vermell/vermell.h>

//
// Application environment (Node.js-style): vermell loads the .env file sitting
// next to the executable automatically. When building this example by hand,
// copy the .env next to your binary:
//
//   g++ -std=c++20 examples/environment/main.cpp -o server -I include -L build -lvermell -pthread
//   cp examples/environment/.env .
//
//   curl http://localhost:9000/
//   curl http://localhost:9000/session
//
// Use `using vermell::environment;` for the short Node-like form.
//
int main() {

    using vermell::environment;

    Router router;
    // PORT comes from the .env (9000); 8080 is the fallback when absent.
    router.setPort(static_cast<uint16_t>(environment.get_as<int>("PORT", 8080)));


    router.get("/",{[](Query &web) {
        web.json(vermell::Json::object({
            {"app_name",   environment.get("APP_NAME", "vermell-app")},
            {"api_token",  environment["API_TOKEN"]}, // operator[] == get() without fallback
            {"debug",      environment.get_as<bool>("DEBUG", false)},
            {"max_conn",   environment.get_as<long>("MAX_CONN", 100)},
            {"env_file",   environment.path()},       // .env file currently in use
            {"keys",       static_cast<long>(environment.size())},
        }).dump());
    }});


    // set()/unset() keep runtime "session" values in memory; they shadow the
    // file and the OS environment and survive environment.reload().
    router.get("/session",{[](Query &web) {
        static int visits = 0;
        environment.set("last_visit", std::to_string(++visits));

        web.send("this process has served /session " +
                 environment.get("last_visit") + " time(s)");
    }});


    std::cout << environment.get("APP_NAME", "vermell-app")
              << " listening on http://localhost:" << environment.get("PORT", "8080") << '\n';

    router.listen();
}
