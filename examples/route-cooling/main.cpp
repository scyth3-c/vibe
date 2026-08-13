#include <vermell/vermell.h>

//
// Route cooling: after the first request the route enters cooldown and
// further requests get a 401 JSON response until the cooldown expires.
//
//   curl -i http://localhost:8080/         # twice in a row
//   curl -i http://localhost:8080/custom
//
int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        web.guard(5); // 5 seconds route cooldown
        web.send("Hello from route cooling");
    }});


    // with a custom cooldown message:
    router.get("/custom",{[&](Query &web) {

        web.guard(10, "slow down, try again in a bit");
        web.send("Hello with a custom cooldown message");
    }});


    router.listen();
}
