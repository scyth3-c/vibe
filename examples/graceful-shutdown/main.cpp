#include <vibe/vibe.h>

#include <iostream>

//
// Lifecycle control: listen() loops until setListenStatus(STOP) is called,
// e.g. from a signal handler or an admin route. The loop wakes up every
// epoll_timeout (1s by default), so the stop is honored within that period.
//
//   curl http://localhost:8080/          # works
//   curl http://localhost:8080/shutdown  # responds and stops the server
//
int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[](Query &web) {
        web.send("try GET /shutdown to stop the server");
    }});


    router.get("/shutdown",{[&](Query &web) {

        web.send("shutting down...");
        // the response is flushed first; the listen loop notices the
        // STOP on its next wake-up and returns.
        router.setListenStatus(neo::STOP);
    }});


    router.listen(); // blocks here until stopped
    std::cout << "server stopped cleanly" << std::endl;

    // For tests or one-shot servers there is also listenOne():
    // it serves a single request and returns.
    //
    //   router.listenOne();
}
