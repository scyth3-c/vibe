#include <vermell/vermell.h>

//
// Raw request bodies: when the client posts JSON, text or binary data
// (anything that is not a form), the body is exposed through raw() and
// its media type through contentType(). For backward compatibility the
// whole body is also available as the ("data", body) parameter.
//
//   curl -X POST http://localhost:8080/json -H "Content-Type: application/json" -d '{"lang":"c++","level":20}'
//   curl -X POST http://localhost:8080/echo -H "Content-Type: text/plain" -d 'plain text body'
//
int main() {

    Router router;
    router.setPort(8080);


    // JSON body: hand raw() to your favorite JSON library.
    router.post("/json",{[](Query &web) {

        if (!web.body.hasBody())
            return web.json(R"({"error":"empty body"})", 400);

        // web.body.contentType() == "application/json" here
        const string raw = web.body.raw(); // {"lang":"c++","level":20}

        web.json(R"({"received":)" + std::to_string(raw.size()) + "}");
    }});


    // Any content type: echo the body back.
    router.post("/echo",{[](Query &web) {

        web.setHeaders("X-Echo-Content-Type: " + string(web.body.contentType()));
        web.send(web.body.raw());
    }});


    // Forms keep working as always: urlencoded bodies are parsed into
    // parameters, everything else arrives as the ("data", body) parameter.
    router.post("/legacy",{[](Query &web) {

        const string data = web.body.getParameters().get("data").value;
        web.send(data);
    }});


    router.listen();
}
