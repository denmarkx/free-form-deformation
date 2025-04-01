#include <iostream>
#include "freeFormDeform.h"
#include "objectHandles.h"

#include "windowFramework.h"
#include "pandaFramework.h"
#include "config_dgraph.h"
#include "config_event.h"

int main() {

    PandaFramework *framework = new PandaFramework();
    framework->open_framework();

    WindowFramework* window = framework->open_window();
    window->setup_trackball();
    window->enable_keyboard();

    NodePath np = window->load_model(window->get_render(), "jack.egg");


    //ObjectHandles oh(np, window->get_mouse(), window->get_camera_group(), window->get_camera(0), window->get_aspect_2d(), window->get_render_2d());
    //oh.set_thickness(5);
    FreeFormDeform* ffd = new FreeFormDeform(np, window->get_render());
    ffd->setup_clicker(*window);
    framework->main_loop();
    return 0;
}
