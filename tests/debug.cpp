#include "../include/vibe/vibe.h"

#include <iomanip>
#include <iostream>

int main()
{
    Router router;
    router.setPort(8080);

    // Configuración para imágenes pesadas: sube el límite de la petición,
    // da más tiempo de lectura entre chunks y ajusta los workers.
    router.configure({
        .read_timeout      = std::chrono::seconds{30},
        .write_timeout     = std::chrono::seconds{10},
        .max_request_size  = 64UL * 1024UL * 1024UL, // 64 MiB
        .read_chunk        = 64UL * 1024UL,
        .threads           = 4,
    });

    // equivalente con setters encadenables:
    // router.setMaxRequestSize(64UL * 1024UL * 1024UL).setReadTimeout(std::chrono::seconds{30});

    router.get("/", {
                   [&](Query& web)
                   {
                       web.send("Hello World, Debug");
                   }
               });

    // Recibe una imagen (multipart/form-data) y muestra sus bytes en consola.
    //
    //   curl -F "image=@foto.png" http://localhost:8080/upload
    //
    router.post("/upload", {
                    [&](Query& web)
                    {
                        const auto* image = web.body.file("image");
                        if (image == nullptr)
                            return web.send("expected a file in the 'image' field", 400);

                        std::cout << "field:    " << image->field << '\n'
                                  << "filename: " << image->filename << '\n'
                                  << "mime:     " << image->mime << '\n'
                                  << "bytes:    " << image->size() << '\n';

                        // vuelco de los bytes recibidos
                        for (const unsigned char byte : image->content)
                            std::cout << std::hex << std::setw(2) << std::setfill('0')
                                      << static_cast<int>(byte) << ' ';
                        std::cout << std::dec << std::endl;

                        web.json(R"({"received":)" + std::to_string(image->size()) + "}");
                    }
                });

    router.listen();
}
