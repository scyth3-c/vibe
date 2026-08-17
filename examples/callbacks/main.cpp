#include <vermell/vermell.h>

int main() {

    Router tasty;
    tasty.setPort(8080);
    string unscoped_info = "123";


    /** for the second request, unscoped_info = "other value"  **/
    tasty.get("/",{[&](Query &web){



        auto params = web.body.getParameters();
        if(params.get("code").value == unscoped_info) {
           return web.json(unscoped_info,[&](){
                unscoped_info = "other value";
            });
        }
            web.json("error", 400, [&](){
                unscoped_info = "error";
            });



    }});




    tasty.listen();

    return 0;
}