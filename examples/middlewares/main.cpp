#include <vermell/vermell.h>

int main() {

    Router router;
    router.setPort(8080);

    std::string auth_id = "123";
    std::string data = "data";

    router.get("/",{
            [&](Query &web) {


                Param_t params = web.body.getParameters();
                auto id = params.get("id");

                if(id.value == auth_id)
                    web.next();
                else
                    web.send("error");


            },
            [&](Query &control) {
                control.send(data);
            }
    });


    router.listen();
}