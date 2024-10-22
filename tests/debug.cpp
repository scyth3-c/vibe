#include "../include/vibe/vibe.h"

int main() {

    Router router;
    router.setPort(8081);

    router.get("/",{[&](Query &web) {
        web.send("Hello World, Debug");
    }});


    router.listen();
}