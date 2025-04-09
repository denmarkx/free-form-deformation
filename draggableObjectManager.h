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

class DraggableObjectManager {
public:
    DraggableObjectManager(NodePath& parent, NodePath& camera_np, NodePath& mouse_np);
    DraggableObjectManager();

    void setup_nodes(NodePath& parent, NodePath& camera_np, NodePath& mouse_np);
    void setup_mouse(std::string click_button);
    void register_object(DraggableObject& draggable);
    void click();
    void deselect_all();

    static DraggableObjectManager* get_global_ptr();

public:
    PT(Camera) _camera;
    PT(MouseWatcher) _mouse;

private:
    void setup_picking_objects();

private:
    NodePath _parent;
    NodePath _camera_np;
    NodePath _mouse_np;

    CollisionTraverser* _traverser;
    PT(CollisionRay) _collision_ray;
    PT(CollisionHandlerQueue) _handler_queue;
    PT(GenericAsyncTask) _clicker_task;

    EventHandler* event_handler = EventHandler::get_global_event_handler();
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();

    ObjectHandles* object_handles;

    std::vector<DraggableObject> _objects;
    std::map<std::string, DraggableObject> _tag_map;

    static DraggableObjectManager* _global_ptr;
};

#endif
