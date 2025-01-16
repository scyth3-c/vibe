#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        int num_modules = 2;
        web.compose("./index.html", num_modules);

    }});


    router.listen();
}