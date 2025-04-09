#include <iostream>
#include "freeFormDeform.h"
#include "objectHandles.h"

#include "windowFramework.h"
#include "pandaFramework.h"
#include "config_dgraph.h"
#include "config_event.h"

void update_edge_span(const Event* e, void* args) {
    FreeFormDeform *_ffd = (FreeFormDeform*)args;
    _ffd->set_edge_spans(5, 5, 5);
}

int main() {

    PandaFramework *framework = new PandaFramework();
    framework->open_framework();

    WindowFramework* window = framework->open_window();
    window->setup_trackball();
    window->enable_keyboard();

    NodePath np = window->load_model(window->get_render(), "teapot");
    //np.flatten_strong();

    ObjectHandles *oh = new ObjectHandles(NodePath(), window->get_mouse(), window->get_camera_group(), window->get_camera(0));
    oh->set_thickness(5);

    FreeFormDeform* ffd = new FreeFormDeform(np, window->get_render());
    ffd->setup_clicker(*window);

    framework->define_key("e", "edge_span_test", update_edge_span, ffd);
    framework->main_loop();
    return 0;
}
