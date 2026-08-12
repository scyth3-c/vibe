#include <vibe/vibe.h>

using vibe::process;
using std::string;

int main() {

    Router router;
    router.setPort(8080);

    router.get("/",{[&](Query &web) {

        string ruta = process.pwd;
        web.readFileX(ruta + "/cpp.html", "text/html");

    }});

    router.listen();
}
