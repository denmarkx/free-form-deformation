#ifndef DRAGGABLE_OBJECT_MANAGER
#define DRAGGABLE_OBJECT_MANAGER

#include "nodePath.h"
#include "collisionTraverser.h"
#include "collisionRay.h"
#include "collisionHandlerQueue.h"
#include "collisionNode.h"
#include "camera.h"
#include "mouseWatcher.h"
#include "genericAsyncTask.h"
#include "asyncTaskManager.h"
#include "draggableObject.h"
#include "objectHandles.h"

#include <unordered_map>

class DraggableObjectManager {
public:
    DraggableObjectManager(NodePath& parent, NodePath& camera_np, NodePath& mouse_np);
    DraggableObjectManager();
    ~DraggableObjectManager();

    void setup_nodes(NodePath& parent, NodePath& camera_np, NodePath& mouse_np);
    void setup_mouse(std::string click_button);
    void register_object(DraggableObject& draggable);
    void click();
    void deselect_all();

    void register_event(DraggableObject& draggable, std::string event_name);
    inline CPT(Event) get_event(DraggableObject& draggable);

    static DraggableObjectManager* get_global_ptr();

public:
    PT(Camera) _camera;
    PT(MouseWatcher) _mouse;
    std::vector<DraggableObject*> _objects;
    LPoint2f _mouse_pos;

    EventHandler* event_handler = EventHandler::get_global_event_handler();

private:
    void setup_picking_objects();

private:
    NodePath _parent;
    NodePath _camera_np;
    NodePath _mouse_np;
    NodePath _picker_node;

    CollisionTraverser* _traverser;
    PT(CollisionRay) _collision_ray;
    PT(CollisionHandlerQueue) _handler_queue;
    PT(GenericAsyncTask) _clicker_task;

    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();

    ObjectHandles* object_handles;

    // tag -> DraggableObject
    std::unordered_map<std::string, DraggableObject*> _tag_map;

    // DraggableObject -> const Event*
    std::unordered_map<DraggableObject*, CPT(Event)> _event_map;

    static DraggableObjectManager* _global_ptr;
};

#endif
