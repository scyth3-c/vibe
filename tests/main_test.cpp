#include "suite.h"


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

