#include <vermell/vermell.h>

//
// vermell::Json: a real JSON tree (DOM) — native types, arrays, proper
// nesting and correct string escaping, with a strict parser included.
//
//   curl http://localhost:8080/
//   curl -X POST http://localhost:8080/sum -H "Content-Type: application/json" -d '{"a": 2, "b": 3.5}'
//
int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[](Query &web) {

        const auto dev = vermell::Json::object({
            {"target", "123"},
            {"lang",   "c++"},
            {"level",  20},          // numbers stay numbers
            {"active", true},        // booleans stay booleans
            {"cache",  nullptr},     // null
            {"tags",   vermell::Json::array({"web", "http", "linux"})},
            {"dev", vermell::Json::object({ // real nesting
                {"name",     "kevin"},
                {"lastname", "bohorquez"},
                {"age",      100},
            })},
        });

        web.json(dev.dump());
    }});


    // parsing: validate and read a JSON body safely
    router.post("/sum",{[](Query &web) {

        const auto body = vermell::Json::parse(web.body.raw());
        if (!body.has_value())
            return web.json(R"({"error":"invalid json"})", 400);

        // at() returns nullptr when the key is absent or not an object
        const auto* a = body->at("a");
        const auto* b = body->at("b");
        if (a == nullptr || b == nullptr || !a->is_number() || !b->is_number())
            return web.json(R"({"error":"expected numbers a and b"})", 400);

        web.json(vermell::Json::object({
            {"result", a->as_double() + b->as_double()},
        }).dump());
    }});


    // The legacy JSON_s facade still compiles, now implemented safely:
    const JSON_s legacy = { "id", "01", "token", "0x4b" };
    router.get("/legacy",{[legacy](Query &web) {
        web.json(legacy());
    }});


    router.listen();
}
