#include "suite.h"

#include "../include/vermell/util/secure_render.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <thread>


 TEST_F(TestSuite, TestBaseOne) {

     Router router;
     router.setPort(8080);
     router.get("/", {[&](Query &http) {
               http.send(expected_default);
       }});

     ISOLATE(
          router.listenOne();
     )

     const string res = http->get();
     isolate_method.get();

     EXPECT_EQ(expected_default, res);
 }



  TEST_F(TestSuite, TestBasePost) {

      Router router;
      router.setPort(8080);

      ISOLATE(
           router.post("/", {[&](Query &http) {
                   http.send(expected_default);
           }});
           router.listenOne();
      )

      const string res = http->post();
      isolate_method.get();

      EXPECT_EQ(res, res);
  }




 TEST_F(TestSuite, TestCompose) {

     Router router;
     router.setPort(8080);
     const string file = "../examples/file-template/index.html";

     ISOLATE(
             router.get("/", {[&](Query &http) {
                 int num_modules = 2;
                http.compose(file, num_modules);
            }});
            router.listenOne();
     )

     const string res = http->get();
     isolate_method.get();

     EXPECT_GT(res.length(), neosys::process::readFile(file).length());
 }



TEST_F(TestSuite, TestReadFile) {

    Router router;
    router.setPort(8080);
    const string file = "../examples/files/test.json";

    ISOLATE(
            router.get("/", {[&](Query &http) {
               http.readFile(file, "application/json");
           }});
           router.listenOne();
    )

     const string response = http->get();
    isolate_method.get();

    EXPECT_EQ(response, neosys::process::readFile(file));
}


 TEST_F(TestSuite, TestCppRender) {

     Router router;
     const string file = "../examples/files/cpp.html";

     router.setPort(8080);
     router.configure({ .render = { .allow_readfilex = true } });
     router.get("/", {[&](Query &http) {
                http.readFileX(file, "application/html");
           }});

     ISOLATE(

            router.listenOne();
     )

     string res = http->get();
     isolate_method.get();

     const std::regex pattern("\\[(.*?)\\]");

     if(std::smatch matches;
        std::regex_search(res, matches, pattern))
        res = matches[1].str() ;


     EXPECT_EQ(res, "hello from c++");
 }



 TEST_F(TestSuite, TestHeaders) {

     Router router;
     router.setPort(8080);
     const string file = "../examples/files/cpp.html";

     router.get("/", {[&](Query &http) {
                auto headers = http.body.getHeaders();
                  HEADERS my_headers = {
                          "header-1: value1",
                          "header-2: value2",
                          "header-3: value3",
                          "header-N: valueN"
                  };
                  http.setHeaders(my_headers);
                  http.send(headers.get("header-1").value);
           }});

     ISOLATE(
            router.listenOne();
     )

     VHeaders my_headers = {
      "header-1: valueX"
     };

     const string res = http->get(my_headers);
     isolate_method.get();

     EXPECT_TRUE(res == "valueX");
 }


 TEST_F(TestSuite, TestMiddleware) {

     Router router;
     router.setPort(8080);

     router.post("/", {

          [&](Query &http) {
                  http.next();
          },
          [&](Query &http) {
              http.send(expected_default);

          }});

     ISOLATE(
          router.listenOne();
     )

     const string res = http->post();
     isolate_method.get();
     EXPECT_TRUE(expected_default == res);
 }



TEST_F(TestSuite, TestParametersQuery) {

     Router router;
     router.setPort(8080);

     router.get("/",{[&](Query &web) {

       auto params = web.body.getParameters();
       if(!params.exist("id")) return web.send("error id not defined");

       const string id_value  = params.get("id").value;

       if( const size_t total = web.body.total_parameters();
           total == 0) return web.send("error with total_parameters");

       web.send("success");

   }});
     ISOLATE(
       router.listenOne();
     )

     const string res = http->get("/?id=2");
     isolate_method.get();

     EXPECT_EQ(res, "success");
 }




TEST_F(TestSuite, TestParametersPost) {

     Router router;
     router.setPort(8080);

     router.post("/",{[&](Query &web) {

       auto params = web.body.getParameters();
       if(!params.exist("id")) return web.send("error id not defined");

       const string id_value  = params.get("id").value;

       if( const size_t total = web.body.total_parameters();
           total == 0) return web.send("error with total_parameters");

       web.send("success");

   }});
     ISOLATE(
       router.listenOne();
     )

     POST fields = {
        "id=2"
     };

     const string res = http->post(fields,"/");
     neosys::process::writeFile("./kevin.res.txt", res);
     isolate_method.get();

     EXPECT_EQ(res, "success");
 }


TEST_F(TestSuite, TestQueryDecoding) {

     Router router;
     router.setPort(8080);

     router.get("/",{[&](Query &web) {
       auto params = web.body.getParameters();
       web.send(params.get("name").value);
   }});
     ISOLATE(
       router.listenOne();
     )

     const string res = http->get("/?name=hello%20world+vermell");
     isolate_method.get();

     EXPECT_EQ(res, "hello world vermell");
 }


TEST_F(TestSuite, TestTypedParameter) {

     Router router;
     router.setPort(8080);

     router.get("/",{[&](Query &web) {
       auto params = web.body.getParameters();
       web.send(std::to_string(params.get("id").as<int>() * 2));
   }});
     ISOLATE(
       router.listenOne();
     )

     const string res = http->get("/?id=21");
     isolate_method.get();

     EXPECT_EQ(res, "42");
 }


TEST_F(TestSuite, TestJsonBody) {

     Router router;
     router.setPort(8080);

     router.post("/",{[&](Query &web) {
       const string legacy_data = web.body.getParameters().get("data").value;
       web.send(web.body.raw() + "|" + legacy_data + "|" + string(web.body.contentType()));
   }});
     ISOLATE(
       router.listenOne();
     )

     const string json_body = R"json({"lang":"c++","level":20})json";
     POST fields = { json_body };
     VHeaders hdrs = { "Content-Type: application/json" };

     const string res = http->post(fields, hdrs, "/");
     isolate_method.get();

     EXPECT_EQ(res, json_body + "|" + json_body + "|application/json");
 }


TEST_F(TestSuite, TestMultipartForm) {

     Router router;
     router.setPort(8080);

     router.post("/",{[&](Query &web) {
       auto params = web.body.getParameters();
       const auto* uploaded = web.body.file("doc");
       if (uploaded == nullptr) return web.send("no file");
       web.send(params.get("title").value + "|" + uploaded->filename + "|" + uploaded->content);
   }});
     ISOLATE(
       router.listenOne();
     )

     const string multipart_body =
         "------vermellTestBoundary\r\n"
         "Content-Disposition: form-data; name=\"title\"\r\n"
         "\r\n"
         "hello vermell\r\n"
         "------vermellTestBoundary\r\n"
         "Content-Disposition: form-data; name=\"doc\"; filename=\"note.txt\"\r\n"
         "Content-Type: text/plain\r\n"
         "\r\n"
         "FILE-CONTENT-123\r\n"
         "------vermellTestBoundary--\r\n";

     POST fields = { multipart_body };
     VHeaders hdrs = { "Content-Type: multipart/form-data; boundary=----vermellTestBoundary" };

     const string res = http->post(fields, hdrs, "/");
     isolate_method.get();

     EXPECT_EQ(res, "hello vermell|note.txt|FILE-CONTENT-123");
 }


TEST_F(TestSuite, TestMimeTypes) {
     EXPECT_EQ(vermell::mime::of("index.html"), "text/html");
     EXPECT_EQ(vermell::mime::of("photo.JPG"), "image/jpeg");
      EXPECT_EQ(vermell::mime::of("script.js"), "application/javascript");
      EXPECT_EQ(vermell::mime::of("kevin.txt.html"), "text/html");
      EXPECT_EQ(vermell::mime::of("PHOTO.JPEG?download=1"), "image/jpeg");
      EXPECT_EQ(vermell::mime::of("feed.json#top"), "application/json");
      EXPECT_EQ(vermell::mime::html, "text/html");
      EXPECT_EQ(vermell::mime::json, "application/json");
      EXPECT_EQ(vermell::mime::of("font.woff2"), "font/woff2");
      EXPECT_EQ(vermell::mime::of("archive.tar.gz"), "application/gzip");
      EXPECT_EQ(vermell::mime::of("data.bin"), "application/octet-stream");
     EXPECT_EQ(vermell::mime::of("no_extension"), "application/octet-stream");
 }


TEST_F(TestSuite, TestResponseBuilder) {
     const string wire = vermell::http::Response{}
                             .status(404)
                             .type("text/plain")
                             .set("X-App", "vermell")
                             .body("oops")
                             .str();

     EXPECT_NE(wire.find("HTTP/1.1 404 Not Found\r\n"), string::npos);
     EXPECT_NE(wire.find("Content-Type: text/plain\r\n"), string::npos);
     EXPECT_NE(wire.find("X-App: vermell\r\n"), string::npos);
     EXPECT_NE(wire.find("Content-Length: 4\r\n"), string::npos);
     EXPECT_TRUE(wire.ends_with("\r\n\r\noops"));
 }


TEST(ThreadPoolBackpressureTest, BlocksWhenQueueIsFullWithoutDroppingWork) {
     threading::ThreadPool pool(1, 1);

     std::atomic<int> completed{0};
     std::atomic<bool> second_started{false};
     std::atomic<bool> producer_finished{false};

     auto first = pool.addTask([&] {
         std::this_thread::sleep_for(std::chrono::milliseconds{60});
         completed.fetch_add(1);
     });

     auto producer = std::async(std::launch::async, [&] {
         auto second = pool.addTask([&] {
             second_started.store(true);
             completed.fetch_add(1);
         });
         second.get();
         producer_finished.store(true);
     });

     std::this_thread::sleep_for(std::chrono::milliseconds{20});
     EXPECT_FALSE(producer_finished.load());

     first.get();
     producer.get();

     EXPECT_TRUE(second_started.load());
     EXPECT_EQ(completed.load(), 2);
 }


TEST_F(TestSuite, TestConfigPayloadTooLarge) {

     Router router;
     router.setPort(8080);

     // Any complete HTTP request is bigger than this: reject with 413.
     router.configure({
         .read_timeout     = std::chrono::seconds{2},
         .max_request_size = 16,
     });

     router.get("/", {[&](Query &http) {
                http.send(expected_default);
       }});

     ISOLATE(
          router.listenOne();
     )

     const string res = http->get();
     isolate_method.get();

     EXPECT_EQ(res, R"lit({"error":"payload too large"})lit");
 }


TEST_F(TestSuite, TestConfigureKeepsFlow) {

     Router router;
     router.setPort(8080);

     router.configure({
         .read_timeout     = std::chrono::seconds{10},
         .write_timeout    = std::chrono::seconds{10},
         .max_request_size = 8UL * 1024UL * 1024UL,
         .read_chunk       = 32UL * 1024UL,
         .threads          = 2,
         .max_events       = 256,
         .max_queue_size   = 64,
     });

     router.get("/", {[&](Query &http) {
                http.send(expected_default);
       }});

     ISOLATE(
          router.listenOne();
     )

     const string res = http->get();
     isolate_method.get();

     EXPECT_EQ(expected_default, res);
     EXPECT_EQ(router.config().port, 8080);
     EXPECT_EQ(router.config().threads, 2UL);
     EXPECT_EQ(router.config().max_queue_size, 64UL);
     EXPECT_EQ(router.config().max_request_size, 8UL * 1024UL * 1024UL);
 }


// ---------------------------------------------------------------------------
// Render hardening
// ---------------------------------------------------------------------------

TEST(SecureRenderUnit, Sha256KnownVector) {
     EXPECT_EQ(vermell::srender::sha256_hex("abc"),
               "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
     EXPECT_EQ(vermell::srender::sha256_hex(""),
               "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SecureRenderUnit, IncludeNameWhitelist) {
     EXPECT_TRUE(vermell::srender::valid_include_name("one.html"));
     EXPECT_TRUE(vermell::srender::valid_include_name("a-b_c.d"));
     EXPECT_FALSE(vermell::srender::valid_include_name("../../etc/passwd"));
     EXPECT_FALSE(vermell::srender::valid_include_name(".."));
     EXPECT_FALSE(vermell::srender::valid_include_name("/etc/passwd"));
     EXPECT_FALSE(vermell::srender::valid_include_name("a/b"));
     EXPECT_FALSE(vermell::srender::valid_include_name(""));
     EXPECT_FALSE(vermell::srender::valid_include_name(string("a\0b", 3)));
}

TEST(SecureRenderUnit, IsWithinJail) {
     EXPECT_TRUE(vermell::srender::is_within(".", "./main_test.cpp"));
     EXPECT_TRUE(vermell::srender::is_within(".", "main_test.cpp"));
     EXPECT_FALSE(vermell::srender::is_within(".", "/etc/passwd"));
     EXPECT_FALSE(vermell::srender::is_within(".", "../../../../etc/passwd"));
     EXPECT_FALSE(vermell::srender::is_within(".", "../vermell/README.md"));
}

TEST(SecureRenderUnit, ResponseHeaderInjectionStripped) {
     const string wire = vermell::http::Response{}
                             .set("X-Safe", "ok\r\nX-Injected: evil")
                             .set("X-Weird\r\nName", "v")
                             .str();
     // The value is truncated at the first CR/LF: no extra header is born.
     EXPECT_EQ(wire.find("X-Injected"), string::npos);
     EXPECT_EQ(wire.find("Name: v"), string::npos);
     EXPECT_NE(wire.find("X-Safe: ok\r\n"), string::npos);
     EXPECT_NE(wire.find("X-Weird: v\r\n"), string::npos);
}

TEST(SecureRenderUnit, GuardRouteEscapesJsonMessage) {
     // A custom guard message with quotes/control chars must not break out
     // of the {"message":"..."} JSON envelope.
     const string wire = utility_t::guard_route(5, "wait\"; injected:\"yes");
     EXPECT_EQ(wire.find("injected:\"yes"), string::npos);
     EXPECT_NE(wire.find("wait\\\"; injected:\\\"yes"), string::npos);
}

TEST(SecureRenderUnit, ReadBoundedAcceptsExactlyMaxBytes) {
     // Off-by-one regression: a file of precisely max_bytes must be served.
     const string file = "./sec_exact.bin";
     { std::ofstream out(file, std::ios::binary); out << string(4096, 'A'); }

     const auto exact = vermell::srender::read_bounded(file, 4096);
     EXPECT_EQ(exact.err, vermell::srender::ReadErr::Ok);
     EXPECT_EQ(exact.data.size(), 4096UL);

     const auto over = vermell::srender::read_bounded(file, 4095);
     EXPECT_EQ(over.err, vermell::srender::ReadErr::TooLarge);
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, BasicReadRejectsNonRegularFiles) {
     // A directory is not a file: the legacy reader served an empty 200.
     auto [data, status] = BasicRead::processing("/tmp");
     EXPECT_EQ(status, "403");
     EXPECT_TRUE(data.empty() || data.find("<") == string::npos || data.find("&lt;") != string::npos);
}

TEST(SecureRenderUnit, BasicReadJailBlocksEscape) {
     vermell::RenderSecurity sec;
     sec.root = "."; // jail to the CWD
     auto [data, status] = BasicRead::processing("/etc/passwd", sec);
     EXPECT_EQ(status, "403");
     EXPECT_EQ(data.find("root:"), string::npos);
}

TEST(SecureRenderUnit, CppReaderWithoutCodeBlockIsVerbatim) {
     // Legacy behavior was undefined (uninitialized coordinates): a template
     // without '$' must now be served untouched.
     const string file = "./sec_notags.html";
     { std::ofstream out(file); out << "<p>plain</p>"; }

     auto [data, status] = CppReader::processing(file);
     EXPECT_EQ(status, "200");
     EXPECT_EQ(data, "<p>plain</p>");
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, CppReaderEmptyFileDoesNotCrash) {
     // Legacy scanned raw_html.length() - 1 == SIZE_MAX positions.
     const string file = "./sec_empty.html";
     { std::ofstream out(file); }

     auto [data, status] = CppReader::processing(file);
     EXPECT_EQ(status, "200");
     EXPECT_TRUE(data.empty());
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, ComposeRejectsTraversalModule) {
     const string file = "./sec_trav.html";
     { std::ofstream out(file); out << "A#[../../../etc/passwd];B"; }

     auto [data, status] = VerReader::processing(file, 1);
     EXPECT_EQ(status, "403");
     EXPECT_EQ(data.find("root:"), string::npos);
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, ComposeWithoutTagsIsUnchanged) {
     // Legacy returned 404 on success and ran with uninitialized coords when
     // the template had no module tag at all.
     const string file = "./sec_plain.html";
     { std::ofstream out(file); out << "<h1>no modules</h1>"; }

     auto [data, status] = VerReader::processing(file, 2);
     EXPECT_EQ(status, "200");
     EXPECT_EQ(data, "<h1>no modules</h1>");
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, DataRenderWithoutMarkersIsUnchanged) {
     // Legacy body_tratament() read/wrote through uninitialized coordinates.
     const string file = "./sec_data_plain.html";
     { std::ofstream out(file); out << "<b>static</b>"; }

     dataRender renderer([](dataRender& d) -> dataRender {
         d("unused", "x");
         return d;
     });
     EXPECT_EQ(renderer.render(file), "<b>static</b>");
     std::filesystem::remove(file);
}

// The new integration tests below use their own port (and their own
// client) so they never share a listening socket with the legacy tests:
// the suite runs every test as a short-lived process on the same port and
// SO_REUSEPORT can hand a fresh connection to a dying neighbor.

TEST_F(TestSuite, TestReadFileXDisabledByConfig) {

     Router router;
     router.setPort(8091);
     router.configure({
         .render = { .allow_readfilex = false },
     });

     router.get("/", {[&](Query &http) {
                http.readFileX("../examples/files/cpp.html", "text/html");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8091");
     const string res = client.get();
     isolate_method.get();

     EXPECT_NE(res.find("disabled"), string::npos);
 }

TEST_F(TestSuite, TestReadFileXKillsInfiniteLoop) {

     const string file = "./sec_loop.html";
     { std::ofstream out(file); out << "X$ while(true){} $Y"; }

      Router router;
      router.setPort(8092);
      router.configure({
          .render = {
              .allow_readfilex = true,
              .run_timeout = std::chrono::milliseconds{500},
          },
      });

     router.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8092");
     const auto start = std::chrono::steady_clock::now();
     const string res = client.get();
     isolate_method.get();
     const auto elapsed = std::chrono::steady_clock::now() - start;

     // The runaway program is SIGKILLed after run_timeout instead of
     // pinning a worker thread forever.
     EXPECT_NE(res.find("failed or timed out"), string::npos);
     EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed).count(), 30);

     // Second request on a fresh router: the cached binary is reused (no
     // recompilation) and the worker pool survived the kill.
      Router router2;
      router2.setPort(8093);
      router2.configure({
          .render = {
              .allow_readfilex = true,
              .run_timeout = std::chrono::milliseconds{500},
          },
      });
     router2.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     Veridic client2("http://localhost:8093");
     const auto start2 = std::chrono::steady_clock::now();
     std::future<void> second = std::async(std::launch::async, [&] { router2.listenOne(); });
     const string res2 = client2.get();
     second.get();
     const auto elapsed2 = std::chrono::steady_clock::now() - start2;

     EXPECT_NE(res2.find("failed or timed out"), string::npos);
     EXPECT_LT(std::chrono::duration_cast<std::chrono::seconds>(elapsed2).count(), 10);

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestReadFileXCacheKeepsOutput) {

     const string file = "./sec_cached.html";
     { std::ofstream out(file); out << "A$ std::cout << \"[cached-ok]\"; $B"; }

      Router router;
      router.setPort(8094);
      router.configure({ .render = { .allow_readfilex = true } });
      router.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

      ISOLATE(
          router.listenOne();
      )

      Veridic client("http://localhost:8094");
      const string res = client.get();
      isolate_method.get();
      EXPECT_NE(res.find("cached-ok"), string::npos);

      // Second hit must produce the exact same output from the cached binary.
      Router router2;
      router2.setPort(8095);
      router2.configure({ .render = { .allow_readfilex = true } });
      router2.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     Veridic client2("http://localhost:8095");
     std::future<void> second = std::async(std::launch::async, [&] { router2.listenOne(); });
     const string res2 = client2.get();
     second.get();
     EXPECT_EQ(res, res2);

      std::filesystem::remove(file);
  }

TEST_F(TestSuite, TestReadFileXMultipleBlocks) {

     // Text before, between and after the blocks must survive byte-exact and
     // each block's stdout must land at its position in the page.
     const string file = "./sec_multi.html";
     { std::ofstream out(file); out << "A$ std::cout << \"1\"; $B$\n std::cout << \"2\";\n$C"; }

      Router router;
      router.setPort(8108);
      router.configure({ .render = { .allow_readfilex = true } });
      router.get("/", {[&](Query &http) {
                http.readFileX(file); // also exercises MIME auto-detection
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8108");
     const string res = client.get();
     isolate_method.get();

     EXPECT_EQ(res, "A1B2C");

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestReadFileXForLoopAndSecondBlock) {

     // The exact shape from the docs: a loop block, then another block.
     const string file = "./sec_buttons.html";
     { std::ofstream out(file); out <<
         "$\n"
         "    for (int i = 0; i < 10; i++) {\n"
         "        std::cout << \"<button> soy un boton, numero: \" << i << \"</button>\";\n"
         "    }\n"
         "$\n"
         "\n"
         "$\n"
         "    std::cout << \"<button>test</button>\";\n"
         "$\n";
     }

      Router router;
      router.setPort(8109);
      router.configure({ .render = { .allow_readfilex = true } });
      router.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8109");
     const string res = client.get();
     isolate_method.get();

     for (int i = 0; i < 10; i++)
         EXPECT_NE(res.find("<button> soy un boton, numero: " + std::to_string(i) + "</button>"),
                   string::npos);
     EXPECT_NE(res.find("<button>test</button>"), string::npos);

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestReadFileXDanglingDollarIsVerbatim) {

     // A '$' without a closing partner is literal text, not a broken
     // template (prices, shell snippets, truncated files).
     const string file = "./sec_dollar.html";
     { std::ofstream out(file); out << "<p>price: $5 and \"quotes\" \\ backslash</p>"; }

     Router router;
     router.setPort(8110);
     router.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8110");
     const string res = client.get();
     isolate_method.get();

     EXPECT_EQ(res, "<p>price: $5 and \"quotes\" \\ backslash</p>");

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestReadFileXEscapesMarkupIntoSource) {

     // Markup full of C++-hostile bytes (quotes, backslashes, newlines)
     // around a live block: the generated translation unit must still
     // compile and the text must round-trip byte-exact.
     const string file = "./sec_escape.html";
     { std::ofstream out(file); out << "<a title=\"x\\y\">\"q\"</a>\n$ std::cout << \"<b>ok</b>\"; $\n<div>\\done\\</div>"; }

      Router router;
      router.setPort(8111);
      router.configure({ .render = { .allow_readfilex = true } });
      router.get("/", {[&](Query &http) {
                http.readFileX(file, "text/html");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8111");
     const string res = client.get();
     isolate_method.get();

     EXPECT_EQ(res, "<a title=\"x\\y\">\"q\"</a>\n<b>ok</b>\n<div>\\done\\</div>");

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestReadFileJailOverHttp) {

     const string file = "./sec_ok.json";
     { std::ofstream out(file); out << "{\"ok\":true}"; }

     Router router;
     router.setPort(8096);
     router.configure({
         .render = { .root = "." }, // jail everything to the CWD
     });
     router.get("/", {[&](Query &http) {
                http.readFile("/etc/passwd", "text/plain");
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8096");
     const string blocked = client.get();
     isolate_method.get();
     EXPECT_EQ(blocked.find("root:"), string::npos);

     // Same jail, a file inside it is still served.
     Router router2;
     router2.setPort(8097);
     router2.configure({
         .render = { .root = "." },
     });
     router2.get("/", {[&](Query &http) {
                http.readFile(file, "application/json");
       }});

     Veridic client2("http://localhost:8097");
     std::future<void> second = std::async(std::launch::async, [&] { router2.listenOne(); });
     const string ok = client2.get();
     second.get();
     EXPECT_EQ(ok, "{\"ok\":true}");

     std::filesystem::remove(file);
 }

TEST_F(TestSuite, TestComposeTraversalOverHttp) {

     const string file = "./sec_http_trav.html";
     { std::ofstream out(file); out << "<p>#[../../../etc/passwd];</p>"; }

     Router router;
     router.setPort(8098);
     router.get("/", {[&](Query &http) {
                http.compose(file, 1);
       }});

     ISOLATE(
          router.listenOne();
     )

     Veridic client("http://localhost:8098");
     const string res = client.get();
     isolate_method.get();

     EXPECT_EQ(res.find("root:"), string::npos);
     EXPECT_NE(res.find("not allowed"), string::npos);

     std::filesystem::remove(file);
 }


// ---------------------------------------------------------------------------
// JSON DOM (vermell::Json)
// ---------------------------------------------------------------------------

TEST(JsonUnit, SerializesNativeTypesInOrder) {
     const auto dev = vermell::Json::object({
         {"name",   "vermell"},
         {"level",  20},
         {"pi",     3.5},
         {"active", true},
         {"cache",  nullptr},
         {"tags",   vermell::Json::array({"web", "http"})},
         {"nested", vermell::Json::object({{"x", 1}})},
     });
     EXPECT_EQ(dev.dump(),
         R"({"name":"vermell","level":20,"pi":3.5,"active":true,"cache":null,"tags":["web","http"],"nested":{"x":1}})");
}

TEST(JsonUnit, SerializesEmptyContainersAndInt64) {
     EXPECT_EQ(vermell::Json::array({}).dump(), "[]");
     EXPECT_EQ(vermell::Json::object({}).dump(), "{}");
     EXPECT_EQ(vermell::Json(nullptr).dump(), "null");
     EXPECT_EQ(vermell::Json(INT64_MAX).dump(), "9223372036854775807");

     // numbers too big for int64 degrade to double on parse
     const auto big = vermell::Json::parse("9223372036854775808");
     ASSERT_TRUE(big.has_value());
     EXPECT_TRUE(big->is_double());
}

TEST(JsonUnit, EscapesStrings) {
     EXPECT_EQ(vermell::Json("a\"b\\c").dump(), R"("a\"b\\c")");
     EXPECT_EQ(vermell::Json("line\nnext\ttab").dump(), R"("line\nnext\ttab")");
     // remaining control chars become \u00XX
     EXPECT_EQ(vermell::Json(string("x\1y", 3)).dump(), "\"x\\u0001y\"");
}

TEST(JsonUnit, TypedAccessAndLookup) {
     const auto j = vermell::Json::parse(R"({"name":"vermell","level":20,"tags":["a","b"],"pi":3.5})");
     ASSERT_TRUE(j.has_value());
     EXPECT_TRUE(j->is_object());
     EXPECT_EQ(j->size(), 4UL);

     const auto* level = j->at("level");
     ASSERT_TRUE(level != nullptr);
     EXPECT_TRUE(level->is_int());
     EXPECT_EQ(level->as_int(), 20);
     EXPECT_EQ(level->as_double(), 20.0);

     EXPECT_EQ(j->at("missing"), nullptr);
     EXPECT_EQ(std::string(j->at("name")->as_string()), "vermell");

     const auto* tags = j->at("tags");
     ASSERT_TRUE(tags != nullptr && tags->is_array());
     EXPECT_EQ(tags->size(), 2UL);
     EXPECT_EQ(std::string(tags->at(1)->as_string()), "b");
     EXPECT_EQ(tags->at(5), nullptr);

     // fallbacks kick in on the wrong type
     EXPECT_EQ(j->at("name")->as_int(7), 7);
     EXPECT_TRUE(j->at("pi")->is_double());
}

TEST(JsonUnit, RoundTripIsStable) {
     const string src = R"({"a":[1,2.5,"x",null,true],"b":{"c":-3},"u":"éè"})";
     const auto first = vermell::Json::parse(src);
     ASSERT_TRUE(first.has_value());
     const auto second = vermell::Json::parse(first->dump());
     ASSERT_TRUE(second.has_value());
     EXPECT_EQ(first->dump(), second->dump());
}

TEST(JsonUnit, UnicodeEscapesToUtf8) {
     const auto j = vermell::Json::parse(R"("é€\uD83D\uDE00")");
     ASSERT_TRUE(j.has_value());
     EXPECT_EQ(std::string(j->as_string()), "é€😀");
}

TEST(JsonUnit, RejectsInvalidJson) {
     EXPECT_FALSE(vermell::Json::parse("").has_value());
     EXPECT_FALSE(vermell::Json::parse("{").has_value());
     EXPECT_FALSE(vermell::Json::parse("[1,]").has_value());
     EXPECT_FALSE(vermell::Json::parse("{\"a\":01}").has_value());  // leading zero
     EXPECT_FALSE(vermell::Json::parse("{\"a\" 1}").has_value());   // missing colon
     EXPECT_FALSE(vermell::Json::parse("\"unterminated").has_value());
     EXPECT_FALSE(vermell::Json::parse("\"bad\\xescape\"").has_value());
     EXPECT_FALSE(vermell::Json::parse("\"\\uD800\"").has_value()); // lone surrogate
     EXPECT_FALSE(vermell::Json::parse("true extra").has_value());  // trailing garbage
     EXPECT_FALSE(vermell::Json::parse("01").has_value());

     // recursion bomb: beyond the depth cap
     const string bomb = string(300, '[') + string(300, ']');
     EXPECT_FALSE(vermell::Json::parse(bomb).has_value());

     // ...but a deep-yet-reasonable tree parses fine
     const string deep = string(200, '[') + string(200, ']');
     EXPECT_TRUE(vermell::Json::parse(deep).has_value());
}

TEST(JsonUnit, LegacyJsonSIsSafeNow) {
     // alternating key/value still compiles; an odd trailing key is null
     // instead of an out-of-bounds read
     const JSON_s legacy = { "id", "01", "level", 20, "odd" };
     EXPECT_EQ(legacy(), R"({"id":"01","level":20,"odd":null})");

     // real nesting through the implicit conversion
     const JSON_s nested = { "user", vermell::Json::object({{"name", "kevin"}}) };
     EXPECT_EQ(nested(), R"({"user":{"name":"kevin"}})");

     // braces inside strings are just data now, not corruption targets
     const JSON_s tricky = { "code", "{ not json }" };
     EXPECT_EQ(tricky(), R"({"code":"{ not json }"})");
}


 TEST_F(TestSuite, TestJsonDomOverHttp) {

      Router router;
      router.setPort(8099);

      router.post("/", {[&](Query &http) {
         const auto body = vermell::Json::parse(http.body.raw());
         if (!body.has_value())
             return http.json(R"({"error":"invalid json"})", 400);

         const auto* a = body->at("a");
         http.json(vermell::Json::object({
             {"double", a != nullptr ? a->as_double() * 2 : 0.0},
         }).dump());
        }});

      ISOLATE(
           router.listenOne();
      )

      Veridic client("http://localhost:8099");
      POST fields = { R"({"a": 21})" };
      VHeaders hdrs = { "Content-Type: application/json" };

      const string res = client.post(fields, hdrs, "/");
      isolate_method.get();

      EXPECT_EQ(res, R"({"double":42})");
 }


// ---------------------------------------------------------------------------
// readFileX toolchain (RenderSecurity::cpp)
// ---------------------------------------------------------------------------

TEST(CppToolchainUnit, StandardIsConfigurable) {
     // A requires-clause on a lambda is a hard error in C++17 mode.
     const string file = "./tc_std.html";
     { std::ofstream out(file); out << "X$ auto f = [](auto x) requires true { return x * 2; }; std::cout << f(21); $Y"; }

      vermell::RenderSecurity sec; // legacy default: c++17
      sec.allow_readfilex = true;
      auto [body17, status17] = CppReader::processing(file, sec);
     EXPECT_EQ(status17, "400");

     sec.cpp.standard = "c++20";
     auto [body20, status20] = CppReader::processing(file, sec);
     EXPECT_EQ(status20, "200");
     EXPECT_EQ(body20, "X42Y");

     std::filesystem::remove(file);
 }

TEST(CppToolchainUnit, ExtraFlagsReachTheCompiler) {
     const string file = "./tc_flags.html";
     { std::ofstream out(file); out << "A$ std::cout << ANSWER; $B"; } // ANSWER undefined by default

      vermell::RenderSecurity sec;
      sec.allow_readfilex = true;
      sec.cpp.compiler = "g++"; // bare names are resolved in the usual dirs
     auto [plain, status_plain] = CppReader::processing(file, sec);
     EXPECT_EQ(status_plain, "400");

     sec.cpp.flags = {"-DANSWER=42"};
     auto [defined, status_defined] = CppReader::processing(file, sec);
     EXPECT_EQ(status_defined, "200");
     EXPECT_EQ(defined, "A42B");

     std::filesystem::remove(file);
 }

TEST(CppToolchainUnit, CacheSeparatesToolchains) {
     // Same source, different flags: the second build must NOT get the
     // first toolchain's cached binary.
     const string file = "./tc_cache.html";
     { std::ofstream out(file); out << "A$ std::cout << ANSWER; $B"; }

      vermell::RenderSecurity sec;
      sec.allow_readfilex = true;
      sec.cpp.flags = {"-DANSWER=1"};
     auto [one, status_one] = CppReader::processing(file, sec);
     EXPECT_EQ(status_one, "200");
     EXPECT_EQ(one, "A1B");

     sec.cpp.flags = {"-DANSWER=2"};
     auto [two, status_two] = CppReader::processing(file, sec);
     EXPECT_EQ(status_two, "200");
     EXPECT_EQ(two, "A2B");

     std::filesystem::remove(file);
 }

TEST(CppToolchainUnit, BadCompilerPathFailsCleanly) {
     const string file = "./tc_bad.html";
     { std::ofstream out(file); out << "A$ std::cout << 1; $B"; }

      vermell::RenderSecurity sec;
      sec.allow_readfilex = true;
      sec.cpp.compiler = "/no/such/g++";
     auto [body, status] = CppReader::processing(file, sec);
     EXPECT_EQ(status, "400");
     EXPECT_NE(body.find("could not be compiled"), string::npos);

     std::filesystem::remove(file);
 }


// ---------------------------------------------------------------------------
// HTTP parser hardening (vermell::http::Message::inspect / parse)
// ---------------------------------------------------------------------------

using Msg     = vermell::http::Message;
using Framing = vermell::http::Message::Framing;

TEST(ParserHardeningUnit, InspectCompleteWithoutBody) {
     const string wire = "GET / HTTP/1.1\r\nHost: x\r\n\r\n";
     const auto i = Msg::inspect(wire);
     EXPECT_EQ(i.framing, Framing::Complete);
     EXPECT_EQ(i.expected, wire.size());
}

TEST(ParserHardeningUnit, InspectWaitsForTheDeclaredBody) {
     const string head = "POST / HTTP/1.1\r\nContent-Length: 10\r\n\r\n";
     const auto i = Msg::inspect(head + "12345"); // 5 of 10 bytes
     EXPECT_EQ(i.framing, Framing::Incomplete);
     EXPECT_EQ(i.expected, head.size() + 10UL);

     const auto done = Msg::inspect(head + "1234567890");
     EXPECT_EQ(done.framing, Framing::Complete);
}

TEST(ParserHardeningUnit, ConflictingContentLengthIsSmuggling) {
     const auto i = Msg::inspect("POST / HTTP/1.1\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n");
     EXPECT_EQ(i.framing, Framing::BadRequest);
}

TEST(ParserHardeningUnit, DuplicateContentLengthWithSameValueIsLegal) {
     const string wire = "POST / HTTP/1.1\r\nContent-Length: 4\r\nContent-Length: 4\r\n\r\npong";
     const auto i = Msg::inspect(wire);
     EXPECT_EQ(i.framing, Framing::Complete);

     const auto msg = Msg::parse(wire);
     ASSERT_TRUE(msg.has_value());
     EXPECT_EQ(msg->body, "pong");
     EXPECT_EQ(msg->content_length(), 4UL);
}

TEST(ParserHardeningUnit, GarbageContentLengthIsRejected) {
     EXPECT_EQ(Msg::inspect("POST / HTTP/1.1\r\nContent-Length: 12x\r\n\r\n").framing, Framing::BadRequest);
     EXPECT_EQ(Msg::inspect("POST / HTTP/1.1\r\nContent-Length: -3\r\n\r\n").framing,  Framing::BadRequest);
     EXPECT_EQ(Msg::inspect("POST / HTTP/1.1\r\nContent-Length: \r\n\r\n").framing,    Framing::BadRequest);
     // beyond size_t: numeric overflow is also a bad request
     EXPECT_EQ(Msg::inspect("POST / HTTP/1.1\r\nContent-Length: 99999999999999999999999999\r\n\r\n").framing,
               Framing::BadRequest);
}

TEST(ParserHardeningUnit, TransferEncodingIsNotSilentlyMisparsed) {
     const auto i = Msg::inspect("POST / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n");
     EXPECT_EQ(i.framing, Framing::NotImplemented);
}

TEST(ParserHardeningUnit, HeaderFloodIsRejected) {
     string wire = "GET / HTTP/1.1\r\n";
     for (int n = 0; n < 101; ++n)
         wire += "X-H" + std::to_string(n) + ": v\r\n";
     wire += "\r\n";
     EXPECT_EQ(Msg::inspect(wire).framing, Framing::TooManyHeaders);

     // same flood without the terminator: rejected early, while reading
     wire.pop_back(); wire.pop_back();
     EXPECT_EQ(Msg::inspect(wire).framing, Framing::TooManyHeaders);
}

TEST(ParserHardeningUnit, ParseAcceptsOnlyStrictRequestLines) {
     EXPECT_TRUE(Msg::parse("GET / HTTP/1.1\r\n\r\n").has_value());
     EXPECT_TRUE(Msg::parse("GET /a/b?x=1 HTTP/1.0\nHost: h\n\n").has_value()); // legacy \n\n head

     EXPECT_FALSE(Msg::parse("").has_value());
     EXPECT_FALSE(Msg::parse("GARBAGE\r\n\r\n").has_value());                    // no spaces at all
     EXPECT_FALSE(Msg::parse("GET /\r\n\r\n").has_value());                      // missing version
     EXPECT_FALSE(Msg::parse("GET  / HTTP/1.1\r\n\r\n").has_value());            // empty target
     EXPECT_FALSE(Msg::parse("GET / HTTP/1.1 EXTRA\r\n\r\n").has_value());       // four parts
     EXPECT_FALSE(Msg::parse("GET / XYZ\r\n\r\n").has_value());                  // bogus version
     EXPECT_FALSE(Msg::parse(string("G\x01""T / HTTP/1.1\r\n\r\n", 20)).has_value()); // control char in method
}

TEST(ParserHardeningUnit, MalformedHeaderLinesAreRejected) {
     EXPECT_FALSE(Msg::parse("GET / HTTP/1.1\r\nNoColonHere\r\n\r\n").has_value());
     EXPECT_FALSE(Msg::parse("GET / HTTP/1.1\r\nBad Name: x\r\n\r\n").has_value()); // space in the name
     EXPECT_TRUE (Msg::parse("GET / HTTP/1.1\r\nGood-Name: x\r\n\r\n").has_value());
}

TEST(ParserHardeningUnit, ObsFoldedHeadersAreRejected) {
     // RFC 9112 §5.2: a line starting with SP/HTAB is an obsolete fold of the
     // previous field and MUST be rejected — never reinterpreted as a new
     // header (behind a proxy that folds differently it is a desync vector).
     EXPECT_FALSE(Msg::parse("GET / HTTP/1.1\r\nHost: x\r\n X-Injected: yes\r\n\r\n").has_value());
     EXPECT_FALSE(Msg::parse("GET / HTTP/1.1\r\nHost: x\r\n\tX-Injected: yes\r\n\r\n").has_value());
     EXPECT_EQ(Msg::inspect("GET / HTTP/1.1\r\nHost: x\r\n X-Injected: yes\r\n\r\n").framing,
               Framing::BadRequest);
}

TEST(ParserHardeningUnit, BodyWithoutContentLengthIsIgnored) {
     // RFC 9112 §6.3: no Content-Length and no Transfer-Encoding = empty body.
     // Bytes after the head are NOT the body (and we close the connection,
     // so they cannot be a pipelined request either).
     const auto msg = Msg::parse("GET / HTTP/1.1\r\nHost: x\r\n\r\nGARBAGE");
     ASSERT_TRUE(msg.has_value());
     EXPECT_TRUE(msg->body.empty());
     EXPECT_EQ(msg->content_length(), 0UL);
}

TEST(ParserHardeningUnit, BodyIsExactlyContentLengthBytes) {
     const auto msg = Msg::parse("POST / HTTP/1.1\r\nContent-Length: 4\r\n\r\npongEXTRA");
     ASSERT_TRUE(msg.has_value());
     EXPECT_EQ(msg->body, "pong");

     // defense in depth: fewer bytes than promised must not parse
     EXPECT_FALSE(Msg::parse("POST / HTTP/1.1\r\nContent-Length: 100\r\n\r\nshort").has_value());
}

TEST(ParserHardeningUnit, MultipartNameMatchesOnlyAtParameterBoundary) {
     // filename BEFORE name: "name=" must not be matched as a substring
     // inside "filename=", or the field name is stolen by the file name.
     const string body =
         "--b\r\n"
         "Content-Disposition: form-data; filename=\"evil.txt\"; name=\"doc\"\r\n"
         "\r\n"
         "DATA\r\n"
         "--b--\r\n";
     const auto mp = vermell::http::parse_multipart(body, "b");
     ASSERT_EQ(mp.files.size(), 1UL);
     EXPECT_EQ(mp.files[0].field, "doc");
     EXPECT_EQ(mp.files[0].filename, "evil.txt");
     EXPECT_EQ(mp.files[0].content, "DATA");

     // the classic order keeps working, and unknown keys stay empty
     EXPECT_EQ(vermell::http::detail::disposition_param(
                   R"(form-data; name="doc"; filename="a.txt")", "name"), "doc");
     EXPECT_EQ(vermell::http::detail::disposition_param(
                   R"(form-data; name="doc"; filename="a.txt")", "filename"), "a.txt");
     EXPECT_TRUE(vermell::http::detail::disposition_param(
                   R"(form-data; name="doc")", "filename").empty());
}

// Sends raw bytes to a one-shot server and returns the raw wire response.
static string raw_exchange(const uint16_t port, const string& bytes) {
     const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
     if (fd < 0)
         return {};

     sockaddr_in addr{};
     addr.sin_family = AF_INET;
     addr.sin_port   = htons(port);
     ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

     if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
         ::close(fd);
         return {};
     }

     size_t sent = 0;
     while (sent < bytes.size()) {
         const ssize_t n = ::send(fd, bytes.data() + sent, bytes.size() - sent, MSG_NOSIGNAL);
         if (n <= 0) {
             ::close(fd);
             return {};
         }
         sent += static_cast<size_t>(n);
     }
     ::shutdown(fd, SHUT_WR); // half-close: no more bytes will come

     string response;
     char buf[4096];
     for (;;) {
         const ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
         if (n <= 0)
             break;
         response.append(buf, static_cast<size_t>(n));
     }
     ::close(fd);
     return response;
}

TEST_F(TestSuite, TestConflictingContentLengthRejected) {
     Router router;
     router.setPort(8100);
     router.post("/", {[&](Query &http) { http.send("unreachable"); }});

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8100, "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 5\r\nContent-Length: 6\r\n\r\n");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 400 Bad Request"), string::npos);
}

TEST_F(TestSuite, TestTransferEncodingRejected) {
     Router router;
     router.setPort(8101);
     router.post("/", {[&](Query &http) { http.send("unreachable"); }});

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8101, "POST / HTTP/1.1\r\nHost: x\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n0\r\n\r\n");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 501 Not Implemented"), string::npos);
}

TEST_F(TestSuite, TestTooManyHeadersRejected) {
     Router router;
     router.setPort(8102);
     router.get("/", {[&](Query &http) { http.send("unreachable"); }});

     string wire = "GET / HTTP/1.1\r\n";
     for (int n = 0; n < 101; ++n)
         wire += "X-H" + std::to_string(n) + ": v\r\n";
     wire += "\r\n";

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8102, wire);
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 431 Request Header Fields Too Large"), string::npos);
}

TEST_F(TestSuite, TestMalformedRequestLineIsNotA404) {
     Router router;
     router.setPort(8103);
     router.get("/", {[&](Query &http) { http.send("unreachable"); }});

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8103, "GARBAGE\r\n\r\n");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 400 Bad Request"), string::npos);
     EXPECT_EQ(res.find("not defined"), string::npos);
}

TEST_F(TestSuite, TestIncompleteBodyRejected) {
     Router router;
     router.setPort(8104);
     router.post("/", {[&](Query &http) { http.send("unreachable"); }});

     ISOLATE( router.listenOne(); )
     // Promises 100 bytes, sends 5 and closes: not "whatever arrived".
     const string res = raw_exchange(8104, "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 100\r\n\r\nshort");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 400 Bad Request"), string::npos);
}

TEST_F(TestSuite, TestHugeDeclaredBodyRejectedEarly) {
     Router router;
     router.setPort(8105);
     router.post("/", {[&](Query &http) { http.send("unreachable"); }});

     ISOLATE( router.listenOne(); )
     // 10 GiB announced, nothing sent: rejected from the head alone.
     const string res = raw_exchange(8105, "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 10737418240\r\n\r\n");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 413"), string::npos);
}

TEST_F(TestSuite, TestDuplicateContentLengthAccepted) {
     Router router;
     router.setPort(8106);
     router.post("/", {[&](Query &http) { http.send(http.body.raw()); }});

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8106, "POST / HTTP/1.1\r\nHost: x\r\nContent-Length: 4\r\nContent-Length: 4\r\n\r\npong");
     isolate_method.get();

     EXPECT_NE(res.find("HTTP/1.1 200 OK"), string::npos);
     EXPECT_TRUE(res.ends_with("pong"));
}

TEST_F(TestSuite, TestGarbageAfterHeadIsNotBody) {
     Router router;
     router.setPort(8107);
     router.get("/", {[&](Query &http) { http.send(http.body.raw().empty() ? "empty" : "leaked"); }});

     ISOLATE( router.listenOne(); )
     const string res = raw_exchange(8107, "GET / HTTP/1.1\r\nHost: x\r\n\r\nGARBAGE");
     isolate_method.get();

     EXPECT_TRUE(res.ends_with("empty"));
}

