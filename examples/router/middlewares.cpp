#include <vermell/vermell.h>


int main() {

    Router router;
    router.setPort(8080);


    MiddlewareList funcs = {

            [](Query &web) { /* validate here */ web.next(); },
            [&](Query &web){  web.send( std::to_string(router.getPort()) );}
            // N middlewares
    };

    router.get("/",  funcs);


    router.listen();
}