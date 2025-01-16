#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        // exist
        auto params = web.body.getParameters();
        if(!params.exist("id")) return web.send("error id not defined");

        // [] operator
        string id_value_m1 = params[0].value;
        // .get(name)
        string id_value_m2  = params.get("id").value;

        // total_parameters
        int total = web.body.total_parameters();

        web.send(id_value_m1 + id_value_m2 + " total: "+ std::to_string(total));

    }});


    router.listen();
}