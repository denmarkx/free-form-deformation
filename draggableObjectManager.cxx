#include "draggableObjectManager.h"

DraggableObjectManager* DraggableObjectManager::_global_ptr = nullptr;

DraggableObjectManager::DraggableObjectManager() {}

/*
Initializes everything for the DraggableObjectManager.
parent (normally rendere) is what the CollisionTraverser
eventually calls traverse on every click.
*/
DraggableObjectManager::DraggableObjectManager(NodePath &parent, NodePath &camera_np, NodePath &mouse_np) {
    setup_nodes(parent, camera_np, mouse_np);
}

/*

*/
DraggableObjectManager::~DraggableObjectManager() {
    // Stop our Drag Task:
    if (task_mgr->find_task("DOM_DragTask") != nullptr) {
        task_mgr->remove(_clicker_task);
    }

    // Remove click event:
    EventHandler* event_handler = EventHandler::get_global_event_handler();
    event_handler->remove_hooks_with(this);

    // Deselect all:
    deselect_all();

    // Remove everyone:
    _picker_node.remove_node();
    delete _traverser;
    delete object_handles;
}

/*
Sets the parent, camera, and mouse and its underlying nodes.
*/
void DraggableObjectManager::setup_nodes(NodePath &parent, NodePath &camera_np, NodePath &mouse_np) {
    _parent = parent;
    _camera_np = camera_np;
    _camera = DCAST(Camera, _camera_np.node());
    _mouse_np = mouse_np;
    _mouse = DCAST(MouseWatcher, mouse_np.node());
    setup_picking_objects();
}

/*
Registers object with the manager.
*/
void DraggableObjectManager::register_object(DraggableObject& draggable) {
    _objects.push_back(&draggable);

    // Check if this has a tag:
    if (draggable.is_watching_tag()) {
        // Keep track:
        _tag_map[draggable.get_tag()] = &draggable;
    }
}

/*
Sets up the traverser, handler, and ray.
*/
void DraggableObjectManager::setup_picking_objects() {
    // Setup CollisionTraverser:
    _traverser = new CollisionTraverser("DOM_Traverser");

    // Collision Node for our ray picker
    PT(CollisionNode) collision_node = new CollisionNode("DOM_PickerNode");

    // Allow it to see all the GeomNodes.
    collision_node->set_from_collide_mask(GeomNode::get_default_collide_mask());

    // Attach to our camera:
    _picker_node = _camera_np.attach_new_node(collision_node);

    // Setup our ray:
    _collision_ray = new CollisionRay();
    collision_node->add_solid(_collision_ray);

    // Then a handler queue:
    _handler_queue = new CollisionHandlerQueue();
    _traverser->add_collider(_picker_node, _handler_queue);

    // also our object handles..
    object_handles = new ObjectHandles(
        NodePath(),
        _mouse_np,
        _camera_np,
        _camera
    );
}

/*

*/
void DraggableObjectManager::click() {
    _collision_ray->set_from_lens(
        _camera,
        _mouse->get_mouse_x(),
        _mouse->get_mouse_y()
    );

    // Traverse on whomever we received from init.
    _traverser->traverse(_parent);
    _handler_queue->sort_entries();

    // We got nothing.
    if (_handler_queue->get_num_entries() == 0) {
        // If the user clicked off, deselect everything
        deselect_all();
        return;
    }

    // Enable our object handles:
    object_handles->set_active(true);

    // Get our top entry:
    PT(CollisionEntry) entry = _handler_queue->get_entry(0);
    NodePath into_np = entry->get_into_node_path().get_parent();

    // Check if we're already selected:
    pvector<NodePath> selected;
    int num_selected = 0;
    bool deselected_flag = false;

    for (DraggableObject* draggable : _objects) {
        selected = draggable->get_selected();

        // While we're doing this, we're keeping track of selected objects.
        num_selected += selected.size();

        if (std::find(selected.begin(), selected.end(), into_np) != selected.end()) {
            // Tell ObjectHandles about us:
            object_handles->remove_node_path(into_np);

            // Deselect.
            draggable->deselect(into_np);
            deselected_flag = true;
            num_selected -= 1;
        }
    }

    // Check if there's anything selected still.
    // ..and that we actually deselected something.
    if (deselected_flag == true) {
        if (num_selected == 0) {
            object_handles->set_active(false);
        }
        return;
    }

    DraggableObject *draggable;

    // Check to see if we have any of special tags:
    for (std::map<std::string, DraggableObject*>::iterator it = _tag_map.begin(); it != _tag_map.end(); it++) {
        if (into_np.has_net_tag(it->first)) {
            draggable = it->second;
            draggable->select(into_np);

            object_handles->add_node_path(into_np);
            return;
        }
    }
    
    // If we made is this far, that means that we didn't have a tag match.
    // Let's compare directly to our managed objects.
    for (DraggableObject* draggable : _objects) {
        if (draggable->has_node(into_np)) {
            draggable->select(into_np);
            object_handles->add_node_path(into_np);
            return;
        }
    }

    // CollisionEntry is a bit too specific in cases where we ask for a parent.
    // This is the last resort if tagging and direct node comparison fails.
    // We begin at into_np and traverse up and see if we run into a match.

    // TODO: few options for optimizing this:
    // a special bitmask for the node? 
    // internally tagging it?

    NodePath& ancestor = into_np;
    for (DraggableObject* draggable : _objects) {
        // We confirmed that we weren't into_np. Traverse up.
        for (size_t i = 0; i < into_np.get_num_nodes()-1; i++) {
            ancestor = into_np.get_ancestor(i);
            // Check if we have you:
            if (draggable->has_node(ancestor)) {
                // Finally.
                draggable->select(ancestor);
                object_handles->add_node_path(ancestor);
                return;
            }
        }
    }

    // If we're here, that means there was no draggable with this node
    // nor was there one with the tag. Stop the object handles.
    object_handles->set_active(false);
    deselect_all();
}


/*
*/
void DraggableObjectManager::deselect_all() {
    // Run deselect on all our managed nodes:
    for (DraggableObject *draggable : _objects) {
        draggable->deselect();
    }

    // Stop our handles:
    object_handles->set_active(false);
}

/*
The function that gets called when the event assigned to
setup_mouse(x) is triggered.
*/
static void handle_mouse_click(const Event* e, void* args) {
    DraggableObjectManager* dom = (DraggableObjectManager*)args;

    // Ignore if there's no mouse.
    if (!dom->_mouse->has_mouse()) {
        return;
    }

    dom->click();
}

/*
Task that executes whenever we are dragging the mouse whilst the event
assigned to setup_mouse(x) is called.

The point of this is to shoot off an event to the DraggableObject if they're listening.
We don't do this from the ObjectHandler for simplicity.
*/
static AsyncTask::DoneStatus drag_task(GenericAsyncTask *task, void* args) {
    DraggableObjectManager* dom = (DraggableObjectManager*)args;

    // Do not do anything if there's no mouse:
    if (!dom->_mouse->has_mouse()) {
        return AsyncTask::DS_again;
    }

    // Don't do anything if there's no movement:
    if (dom->_mouse_pos == dom->_mouse->get_mouse()) {
        return AsyncTask::DS_again;
    }

    // Set _mouse_pos:
    dom->_mouse_pos = dom->_mouse->get_mouse();

    // Dispatch the event to everyone listening:
    for (DraggableObject *draggable : dom->_objects) {
        CPT(Event) e = new Event(draggable->get_event_name());
        dom->event_handler->dispatch_event(e);
    }
    return AsyncTask::DS_cont;
}

/*
Setups the bind and dragging task for the mouse.
*/
void DraggableObjectManager::setup_mouse(std::string click_button) {
    event_handler->add_hook(click_button, handle_mouse_click, this);

    // Setup our dragging task too:
    _clicker_task = new GenericAsyncTask("DOM_DragTask", &drag_task, this);
    task_mgr->add(_clicker_task);
}

/*

*/
DraggableObjectManager* DraggableObjectManager::get_global_ptr() {
    if (_global_ptr == nullptr) {
        _global_ptr = new DraggableObjectManager();
    }
    return _global_ptr;
}
