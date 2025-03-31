#include "objectHandles.h"

ObjectHandles::ObjectHandles(NodePath np, NodePath mouse_np, NodePath camera_np, Camera *camera, NodePath aspect2d, NodePath render2d) : NodePath("ObjectHandles") {
    _np = np;
    _camera_np = camera_np;
    _camera = camera;
    _mouse_np = mouse_np;
    _active_line_np = NodePath();

    // This is for disabling / re-enabling the camera:
    _trackball = DCAST(Trackball, mouse_np.get_child(0).node());
    _trackball2Cam = DCAST(Transform2SG, _trackball->get_child(0));

    // Convert to MouseWatcher:
    _mouse_watcher = DCAST(MouseWatcher, mouse_np.node());

    rebuild();
    reparent_to(np);
    setup_mouse_watcher();

    // Object Handles are always seen regardless if its behind a solid object.
    set_depth_write(false);
    set_depth_test(false);
    set_bin("fixed", 1, 1);
}

void ObjectHandles::rebuild() {
    // Delete if we had any in our vec:
    for (NodePath& axis_np : _axis_nps) {
        axis_np.remove_node();
    }
    _axis_nps.clear();

    LineSegs line_segs = LineSegs();
    NodePath line_seg_np;
    LPoint3f draw_point(0);
    LColor color(0, 0, 0, 1);

    for (int i = 0; i <= 2; i++) {
        // Set draw/color at i to 1.
        draw_point[i] = get_length();
        color[i] = 1;

        line_segs.move_to(0, 0, 0);
        line_segs.draw_to(draw_point);
        line_segs.set_thickness(get_thickness());

        line_seg_np = attach_new_node(line_segs.create());
        line_seg_np.set_tag("axis", std::to_string(i));

        // On hover, we set the color via set_color on the NP.
        // That's the only reason why we don't use line_segs.set_color..
        // ..unless we want to rebuild it every time
        line_seg_np.set_color_scale(color);

        // Reset:
        draw_point = LPoint3f(0);
        color = LColor(0, 0, 0, 1);

        _axis_nps.push_back(line_seg_np);
    }
}

/*
    
*/
LPoint2f ObjectHandles::get_screen_space_origin(LPoint3f origin, LMatrix4 &proj_mat) {
    LPoint3f point_3d = _camera_np.get_relative_point(*this, origin);

    // From: tarsierpi on p3d forums:
    LVecBase4f point_cam_2d = proj_mat.xform(LVecBase4f(point_3d[0], point_3d[1], point_3d[2], 1.0));
    double point_cam_2d_3 = 1.0 / point_cam_2d[3];

    return LPoint2f(
        point_cam_2d[0] * point_cam_2d_3,
        point_cam_2d[1] * point_cam_2d_3
    );
}

AsyncTask::DoneStatus ObjectHandles::mouse_task(GenericAsyncTask* task, void* args) {
    // Args is the ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)args;

    // Ignore if we have clicked one a line:
    if (!o_handle->_active_line_np.is_empty()) {
        return AsyncTask::DS_again;
    }

    // Get Lens from o_handle->camera
    PT(Lens) lens = o_handle->_camera->get_lens();
    LMatrix4 proj_mat = lens->get_projection_mat();

    double distance, full_segment_dist;

    LPoint2f point_2d, mouse_xy, origin_2d;
    LPoint3f point_3d, origin;

    // Get the mouse coordinates.
    mouse_xy = o_handle->_mouse_watcher->get_mouse();

    // Get the origin of the handles in screen space.
    origin_2d = o_handle->get_screen_space_origin(LPoint3f(0.0001), proj_mat);

    // We're going to map the axis point from render -> render2d. This will allow us to
    // test it against the mouse coordinates instead of doing a collision test.
    int i = 0;
    int _2d_index = 0;

    // Reset _hover_line_np:
    o_handle->_hover_line_np = NodePath();

    for (NodePath& axis_np : o_handle->_axis_nps) {
        LColor color(0, 0, 0, 1);
        color[i] = 1.0;

        origin = LPoint3f(0);
        origin[i] = o_handle->get_length();

        // Reset color:
        axis_np.set_color_scale(color);

        // Object -> Camera Space
        point_3d = o_handle->_camera_np.get_relative_point(axis_np, origin);

        // From: tarsierpi on p3d forums:
        LVecBase4f point_cam_2d = proj_mat.xform(LVecBase4f(point_3d[0], point_3d[1], point_3d[2], 1.0));

        // point_cam_2d[3] provides us the forward "distance" from the object to the camera. We use this to scale our other 2 axis.
        double point_cam_2d_3 = 1.0 / point_cam_2d[3];
        
        // Ignore point_cam_2d[2]
        point_2d = LPoint2f(
            point_cam_2d[0] * point_cam_2d_3,
            point_cam_2d[1] * point_cam_2d_3
        );

        // Distance between end of line segment to the origin.
         full_segment_dist = (point_2d - origin_2d).length();
         full_segment_dist = round(full_segment_dist * 100.0) / 100.0;

         // Distance between mouse to origin + mouse to end of lineseg.
        distance = (point_2d - mouse_xy).length() + (origin_2d - mouse_xy).length();
        distance = round(distance * 100.0) / 100.0;

        // These should be equal (roughly), but we round to ignore precision.
        if (distance != full_segment_dist) {
            i++;
            continue;
        }

        // Set "active":
        o_handle->_hover_line_np = axis_np;

        // Otherwise, we're officially within range.
        axis_np.set_color_scale(1, 1, 0, 1);
        i++;
    }
    return AsyncTask::DoneStatus::DS_cont;
}

AsyncTask::DoneStatus ObjectHandles::mouse_drag_task(GenericAsyncTask* task, void* args) {
    // Args is ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)(args);

    // We can get the axis of our handle via a tag it has:
    int axis = atoi(o_handle->_active_line_np.get_net_tag("axis").c_str());

    // We modify the plane's normal to determine what axis we're intersecting.
    // This'll be useful for not only single axis movements, but 2-axis ones too.
    LVector3f normal = LVector3f(0, 0, 1);

    // XXX
    if (axis == 2) {
        normal = LVector3f(0, -1, 0);
    }

    // Get world space of the object handle:
    LPlanef plane = LPlanef(normal, LPoint3f(0, 0, 0));
    LPoint3f _near, _far, pos3d;

    NodePath parent = o_handle->get_parent();

    // https://discourse.panda3d.org/t/super-fast-mouse-ray-collisions-with-ground-plane/5022
    LPoint3f pos = o_handle->_np.get_pos(parent);

    o_handle->_camera->get_lens()->extrude(o_handle->_mouse_watcher->get_mouse(), _near, _far);

    // Check for intersection:
    if (plane.intersects_line(pos3d,
        parent.get_relative_point(o_handle->_camera_np, _near),
        parent.get_relative_vector(o_handle->_camera_np, _far))) {

        pos[axis] = pos3d[axis];
        o_handle->_np.set_pos(parent, pos);
    }

    return AsyncTask::DS_cont;
}

void ObjectHandles::handle_drag_done(const Event* event, void* args) {
    // Args is ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)(args);

    // Ignore if there is no _active_line_np:
    if (o_handle->_active_line_np.is_empty()) {
        return;
    }

    // Stop dragging task:
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    task_mgr->remove(o_handle->_drag_task);

    // Set _active_line_np to empty.
    o_handle->_active_line_np.clear_color();
    o_handle->_active_line_np = NodePath();

    // Update camera movement:
    o_handle->enable_camera_movement();
}

void ObjectHandles::handle_click(const Event* event, void* args) {
    // Args is ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)(args);

    NodePath active_line = o_handle->_hover_line_np;

    // Ignore if there is no _hover_line_np:
    if (active_line.is_empty()) {
        return;
    }

    // We can go ahead and reset this:
    o_handle->_hover_line_np = NodePath();

    // We use this as a flag to pause mouse_task.
    o_handle->_active_line_np = active_line;

    // Start our dragging task.
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    AsyncTask* _drag_task = new GenericAsyncTask("ObjectHandles_MouseDragTask", mouse_drag_task, o_handle);
    o_handle->_drag_task = _drag_task;
    task_mgr->add(_drag_task);

    // Ignore camera movement:
    o_handle->disable_camera_movement();

}

void ObjectHandles::setup_mouse_watcher() {
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    
    // Mouse task for highlighting (dragging is separate for readability purposes):
    AsyncTask* task = new GenericAsyncTask("ObjectHandles_MouseTask", mouse_task, this);
    task_mgr->add(task);

    // Click event for when we actually click a control.
    EventHandler* event_handler = EventHandler::get_global_event_handler();
    event_handler->add_hook("mouse1", handle_click, this);

    // mouse1-up signals that we are done.
    event_handler->add_hook("mouse1-up", handle_drag_done, this);
}

/*
Disables mouse movement of the camera.
*/
void ObjectHandles::disable_camera_movement() {
    NodePath first_child = _mouse_np.get_child(0);
    if (first_child.get_name() != "trackball") {
        return;
    }
    _trackball_node = _trackball2Cam->get_node();
    _trackball2Cam->set_node(nullptr);

    // Trackball is still made away of incoming events, but we don't actually want to keep those
    // when we re-enable the camera.
    _cam_mat = _trackball->get_mat();
}

/*
Enables mouse movement of the camera.
*/
void ObjectHandles::enable_camera_movement() {
    _trackball2Cam->set_node(_trackball_node);
    _trackball->set_mat(_cam_mat);
}


void ObjectHandles::set_length(double length) {
    _length = length;
    rebuild();
}

double ObjectHandles::get_length() {
    return _length;
}

void ObjectHandles::set_thickness(double thickness) {
    _thickness = thickness;
    rebuild();
}

double ObjectHandles::get_thickness() {
    return _thickness;
}
