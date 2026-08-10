#include "suite.h"

#include "../include/vibe/util/secure_render.h"

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

     const string res = http->get("/?name=hello%20world+vibe");
     isolate_method.get();

     EXPECT_EQ(res, "hello world vibe");
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
         "------vibeTestBoundary\r\n"
         "Content-Disposition: form-data; name=\"title\"\r\n"
         "\r\n"
         "hello vibe\r\n"
         "------vibeTestBoundary\r\n"
         "Content-Disposition: form-data; name=\"doc\"; filename=\"note.txt\"\r\n"
         "Content-Type: text/plain\r\n"
         "\r\n"
         "FILE-CONTENT-123\r\n"
         "------vibeTestBoundary--\r\n";

     POST fields = { multipart_body };
     VHeaders hdrs = { "Content-Type: multipart/form-data; boundary=----vibeTestBoundary" };

     const string res = http->post(fields, hdrs, "/");
     isolate_method.get();

     EXPECT_EQ(res, "hello vibe|note.txt|FILE-CONTENT-123");
 }


TEST_F(TestSuite, TestMimeTypes) {
     EXPECT_EQ(vibe::mime::of("index.html"), "text/html");
     EXPECT_EQ(vibe::mime::of("photo.JPG"), "image/jpeg");
     EXPECT_EQ(vibe::mime::of("script.js"), "application/javascript");
     EXPECT_EQ(vibe::mime::of("data.bin"), "application/octet-stream");
     EXPECT_EQ(vibe::mime::of("no_extension"), "application/octet-stream");
 }


TEST_F(TestSuite, TestResponseBuilder) {
     const string wire = vibe::http::Response{}
                             .status(404)
                             .type("text/plain")
                             .set("X-App", "vibe")
                             .body("oops")
                             .str();

     EXPECT_NE(wire.find("HTTP/1.1 404 Not Found\r\n"), string::npos);
     EXPECT_NE(wire.find("Content-Type: text/plain\r\n"), string::npos);
     EXPECT_NE(wire.find("X-App: vibe\r\n"), string::npos);
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
     EXPECT_EQ(vibe::srender::sha256_hex("abc"),
               "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
     EXPECT_EQ(vibe::srender::sha256_hex(""),
               "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SecureRenderUnit, IncludeNameWhitelist) {
     EXPECT_TRUE(vibe::srender::valid_include_name("one.html"));
     EXPECT_TRUE(vibe::srender::valid_include_name("a-b_c.d"));
     EXPECT_FALSE(vibe::srender::valid_include_name("../../etc/passwd"));
     EXPECT_FALSE(vibe::srender::valid_include_name(".."));
     EXPECT_FALSE(vibe::srender::valid_include_name("/etc/passwd"));
     EXPECT_FALSE(vibe::srender::valid_include_name("a/b"));
     EXPECT_FALSE(vibe::srender::valid_include_name(""));
     EXPECT_FALSE(vibe::srender::valid_include_name(string("a\0b", 3)));
}

TEST(SecureRenderUnit, IsWithinJail) {
     EXPECT_TRUE(vibe::srender::is_within(".", "./main_test.cpp"));
     EXPECT_TRUE(vibe::srender::is_within(".", "main_test.cpp"));
     EXPECT_FALSE(vibe::srender::is_within(".", "/etc/passwd"));
     EXPECT_FALSE(vibe::srender::is_within(".", "../../../../etc/passwd"));
     EXPECT_FALSE(vibe::srender::is_within(".", "../vibe/README.md"));
}

TEST(SecureRenderUnit, ResponseHeaderInjectionStripped) {
     const string wire = vibe::http::Response{}
                             .set("X-Safe", "ok\r\nX-Injected: evil")
                             .set("X-Weird\r\nName", "v")
                             .str();
     // The value is truncated at the first CR/LF: no extra header is born.
     EXPECT_EQ(wire.find("X-Injected"), string::npos);
     EXPECT_EQ(wire.find("Name: v"), string::npos);
     EXPECT_NE(wire.find("X-Safe: ok\r\n"), string::npos);
     EXPECT_NE(wire.find("X-Weird: v\r\n"), string::npos);
}

TEST(SecureRenderUnit, BasicReadRejectsNonRegularFiles) {
     // A directory is not a file: the legacy reader served an empty 200.
     auto [data, status] = BasicRead::processing("/tmp");
     EXPECT_EQ(status, "403");
     EXPECT_TRUE(data.empty() || data.find("<") == string::npos || data.find("&lt;") != string::npos);
}

TEST(SecureRenderUnit, BasicReadJailBlocksEscape) {
     vibe::RenderSecurity sec;
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

     auto [data, status] = MgReader::processing(file, 1);
     EXPECT_EQ(status, "403");
     EXPECT_EQ(data.find("root:"), string::npos);
     std::filesystem::remove(file);
}

TEST(SecureRenderUnit, ComposeWithoutTagsIsUnchanged) {
     // Legacy returned 404 on success and ran with uninitialized coords when
     // the template had no module tag at all.
     const string file = "./sec_plain.html";
     { std::ofstream out(file); out << "<h1>no modules</h1>"; }

     auto [data, status] = MgReader::processing(file, 2);
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

