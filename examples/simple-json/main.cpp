#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);

    // The use of JSON_s is not recommended
    // Whenever you can, install or create your library for specialized json management.

    router.get("/",{[&](Query &web) {
        JSON_s id = {
                "id", "01",
                "token", "0x4b"
        },

                kevin = {
                "name", "kevin",
                "lastname", "bohorquez",
                "age", "100",
                "id", id()
        },

                dev = {
                "target", "123",
                "lang",  "c++",
                "dev",  kevin()
        };
        web.json(dev());
    }});


    router.listen();
}