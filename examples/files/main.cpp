#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        web.readFile("./test.json", "application/json");
    }});



    router.get("/cpp",{[&](Query &web) {

        web.readFileX("cpp.html", "text/html");
    }});



    router.get("/stress",{[&](Query &web) {

        web.readFile("./stress.json", "application/json");
    }});


    router.listen();
}