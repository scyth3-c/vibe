#include "../include/vibe/vibe.h"

int main() {

    Router router;
    router.setPort(8081);
    terminal("procesando peticiones... puerto 8081, clock", CLOCK_SPEED);

    router.get("/",{[&](Query &web) {

        // web.guard(milliseconds(10000));
        web.send("procesamiento completado.");
    }});

    router.listen();

    return 0;
}