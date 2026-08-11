#include <vibe/vibe.h>

//
// Parameter access methods.
//
//   curl "http://localhost:8080/?id=21"
//   curl "http://localhost:8080/typed?id=21&active=true&page=3"
//
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


    // typed conversion and fallbacks
    router.get("/typed",{[&](Query &web) {

        auto params = web.body.getParameters();

        // as<T>(fallback): arithmetic types and bool
        int  id     = params.get("id").as<int>();      // 0 when missing or not a number
        bool active = params.get("active").as<bool>(); // 1/true/on/yes => true

        // value_or: fallback string when the parameter is absent
        string page = params.value_or("page", "1");

        // empty(): true when the parameter has no value
        if (params.get("id").empty())
            return web.send("id has no value", 400);

        web.send("id*2=" + std::to_string(id * 2)
               + " active=" + (active ? "true" : "false")
               + " page=" + page);
    }});


    router.listen();
}
