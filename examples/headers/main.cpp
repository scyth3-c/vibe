#include <vermell/vermell.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {


        // get headers
        auto headers = web.body.getHeaders();

        // set
        HEADERS my_headers = {
                "header-1: value1",
                "header-2: value2",
                "header-3: value3",
                "header-N: valueN"
        };
        web.setHeaders(my_headers);

        // alternative
        //  control.setHeaders("header-1: value");

        web.send(headers.get("Accept").value);
    }});


    router.listen();
}