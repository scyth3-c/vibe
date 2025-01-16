#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        web.guard(5); // 5 seconds route cooldown
        web.send("Hello from route cooling");
    }});


    router.listen();
}