#include <vermell/vermell.h>

int main() {

    Router router;
    router.setPort(8080);


    router.get("/",{[&](Query &web) {

        web.render("./data.html",[&](dataRender &Data){

            Data("name", "kevin");  // [[name]] in html file
            Data("age", "21");      // [[age]]  in html file

            return Data;
        });

    }});


    router.listen();
}