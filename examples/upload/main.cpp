#include <vermell/vermell.h>

#include <iostream>

//
// File uploads with multipart/form-data: plain fields land in
// getParameters() and the files are exposed through body.files()
// and body.file(field).
//
//   curl -F "title=hello vermell" -F "doc=@note.txt" http://localhost:8080/upload
//   curl -F "a=@one.txt" -F "b=@two.txt" http://localhost:8080/upload-many
//
int main() {

    Router router;
    router.setPort(8080);

    // Raise the request cap when you expect heavy uploads:
    router.setMaxRequestSize(64UL * 1024UL * 1024UL); // 64 MiB


    router.post("/upload",{[](Query &web) {

        auto params = web.body.getParameters(); // plain form fields
        const string title = params.value_or("title", "untitled");

        // first file uploaded under the "doc" field, nullptr when absent
        const auto* doc = web.body.file("doc");
        if (doc == nullptr)
            return web.send("expected a file in the 'doc' field", 400);

        std::cout << "field:    " << doc->field    << '\n'
                  << "filename: " << doc->filename << '\n'
                  << "mime:     " << doc->mime     << '\n'
                  << "bytes:    " << doc->size()   << '\n';

        // persist it (the target directory must exist). filename is already
        // sanitized by the multipart parser (no path components, no ".."):
        // doc->save_to("./uploads/" + doc->filename);

        web.json(vermell::Json::object({
            {"title", title},
            {"file", doc->filename},
            {"bytes", static_cast<long long>(doc->size())},
        }).dump());
    }});


    // every file of the request, whatever the field name
    router.post("/upload-many",{[](Query &web) {

        if (!web.body.hasFiles())
            return web.send("no files", 400);

        string listing;
        for (const auto &f : web.body.files())
            listing += f.field + ": " + f.filename
                     + " (" + std::to_string(f.size()) + " bytes)\n";

        web.send("total: " + std::to_string(web.body.total_files()) + "\n" + listing);
    }});


    router.listen();
}
