#include <iostream>
#include "freeFormDeform.h"
#include "objectHandles.h"

#include "windowFramework.h"
#include "pandaFramework.h"
#include "config_dgraph.h"
#include "config_event.h"

#include "draggableObjectManager.h"
#include "draggableObject.h"

void update_edge_span(const Event* e, void* args) {
    FreeFormDeform *_ffd = (FreeFormDeform*)args;
    _ffd->set_edge_spans(2, 3, 2);
}

void ls(const Event* e, void* args) {
    WindowFramework* window = (WindowFramework*)args;
    window->get_render().ls();
}

void lattice_debug(const Event* e, void* args) {
    FreeFormDeform* _ffd = (FreeFormDeform*)args;
    std::cout << *_ffd << "\n\n";
    std::cout << _ffd->get_lattice() << "\n";
}

void task_event_debug(const Event* e) {
    EventHandler *eh = EventHandler::get_global_event_handler();
    eh->write(nout);
    std::cout << "\n";

    AsyncTaskManager* tm = AsyncTaskManager::get_global_ptr();
    tm->write(nout);
    std::cout << "\n";
}

int main() {

    PandaFramework *framework = new PandaFramework();
    framework->open_framework();

    WindowFramework* window = framework->open_window();
    window->setup_trackball();
    window->enable_keyboard();

    NodePath np = window->load_model(window->get_render(), "teapot");
    //np.flatten_strong();

    FreeFormDeform* ffd = new FreeFormDeform(np, window->get_render());

    DraggableObjectManager* dom = DraggableObjectManager::get_global_ptr();
    dom->setup_nodes(
        window->get_render(),
        window->get_camera_group().find("**/camera"),
        window->get_mouse()
     );
    dom->setup_mouse("shift-mouse1");

    DraggableObject* draggable = new DraggableObject(np, 0);
    dom->register_object(*draggable);
    draggable->hook_drag_event("d", ffd->handle_drag, ffd);

    framework->define_key("e", "edge_span_test", update_edge_span, ffd);

    framework->define_key("l", "ls", ls, window);
    framework->define_key("c", "lattice_Debug", lattice_debug, ffd);
    framework->define_key("t", "task_event_debug", task_event_debug);

    framework->main_loop();
    return 0;
}
