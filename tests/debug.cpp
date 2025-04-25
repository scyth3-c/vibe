#include "../include/vibe/vibe.h"

int main() {

    Router router;
    router.setPort(8081);

    router.get("/",{[&](Query &web) {

        // web.guard(milliseconds(10000));
        web.send("Hello World, Debug 1");
    }});

    router.listenOne();


    return 0;
}