#include <vibe/vibe.h>

int main() {

    Router router;
    router.setPort(8080);




    router.post("/post",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.put("/put",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.get("/get",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});



    router.deleteX("/delete",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.patch("/patch",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});




    router.head("/head",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});



    router.options("/options",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.link("/link",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.unlink("/unlink",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});


    router.purge("/purgue",{[](Query &control){

        auto params = control.body.getParameters();
        control.json(params.get("id").value);
    }});




    router.listen();
}