#include <vibe/vibe.h>


int main() {

    Router router;
    router.setPort(8080);



    // for example:  myRoutes.h
    MiddlewareList send = {
            [](Query &web) { web.send("hello from /users"); }
    };

    Route_t users { "/users", send, GET_TYPE };


    // main.cpp
    router.use(users);


    router.listen();
}