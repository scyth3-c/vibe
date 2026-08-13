#include <vermell/vermell.h>


int main() {

    Router router;
    router.setPort(8080);

    router.get("/",{[](Query &http) {
                       http.readFileX(vermell::process.pwd + "/cpp.html");
                   }
               });


    router.listen();
}
