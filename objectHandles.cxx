#include "objectHandles.h"

ObjectHandles::ObjectHandles(NodePath &np, NodePath mouse_np, NodePath camera_np, Camera *camera) : NodePath("ObjectHandles") {
    set_active(false);

    // Holder for all of our selected NPs:
    _np_obj_node = new NodePath("np_holder");
    _np_obj_node->reparent_to(*this);

    // Holder for our plane and line axis controls:
    _control_node = new NodePath("control_holder");
    _control_node->reparent_to(*this);
    
    // Add the np if we were given it:
    add_node_path(np);

    _camera_np = camera_np;
    _camera = camera;
    _mouse_np = mouse_np;
    _active_line_np = NodePath();

    // This is for disabling / re-enabling the camera:
    _trackball = DCAST(Trackball, mouse_np.get_child(0).node());
    _trackball2Cam = DCAST(Transform2SG, _trackball->get_child(0));

    // Convert to MouseWatcher:
    _mouse_watcher = DCAST(MouseWatcher, mouse_np.node());
    setup_mouse_watcher();

    // Object Handles are always seen regardless if its behind a solid object.
    set_depth_write(false);
    set_depth_test(false);
    set_bin("fixed", 1, 1);
}

ObjectHandles::~ObjectHandles() {
    cleanup();

    // Stop our tasks:
    PT(AsyncTask) task;
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();

    // Drag Task:
    task = task_mgr->find_task(DRAG_TASK_NAME);
    if (task != nullptr) {
        task_mgr->remove(task_mgr->find_task(DRAG_TASK_NAME));
    }

    // Mouse Task:
    task = task_mgr->find_task(MOUSE_TASK_NAME);
    if (task != nullptr) {
        task_mgr->remove(task_mgr->find_task(MOUSE_TASK_NAME));
    }

    // Ignore mouse1 and mouse1-up:
    EventHandler* event_handler = EventHandler::get_global_event_handler();
    event_handler->remove_hooks_with(this);
}

void ObjectHandles::cleanup() {
    // Detach _np_obj_node's children:
    for (size_t i = 0; i < _np_obj_node->get_num_children(); i++) {
        _np_obj_node->get_child(i).detach_node();
    }

    // Remove our holders:
    _np_obj_node->remove_node();
    _control_node->remove_node();

    delete _np_obj_node;
    delete _control_node;

    // Now remove ourselves:
    remove_node();
}

NodePath ObjectHandles::create_plane_np(LPoint3f pos, LPoint3f hpr, LColor color, AxisType axis_type) {
    // CardMaker is for visualization:
    CardMaker cm = CardMaker("axis_plane");
    cm.set_frame_fullscreen_quad();

    NodePath plane_np = NodePath(cm.generate());
    plane_np.set_tag("axis", std::to_string((int)axis_type));
    plane_np.set_scale(0.03);
    plane_np.set_pos(pos);
    plane_np.set_hpr(hpr);
    plane_np.set_color(color);
    plane_np.set_two_sided(1);

    _axis_plane_nps.push_back(plane_np);
    return plane_np;

}

void ObjectHandles::rebuild() {
    // Reset everything active:
    _hover_line_np = NodePath();
    _active_line_np = NodePath();

    // Delete if we had any in our vec:
    for (NodePath& axis_np : _axis_nps) {
        axis_np.remove_node();
    }
    _axis_nps.clear();

    // Delete the planes to:
    for (NodePath& axis_plane_np : _axis_plane_nps) {
        axis_plane_np.remove_node();
    }
    _axis_plane_nps.clear();

    LineSegs line_segs = LineSegs();
    NodePath line_seg_np;
    LPoint3f draw_point(0);
    LColor color(0, 0, 0, 1);

    // Loose YZ Plane
    NodePath loose_yz_plane = create_plane_np(LPoint3f(0, 0.5, 0.5), LPoint3f(90, 0, 0), LColor(1, 0, 0, 1), AxisType::AT_yz);

    // Loose XZ Plane
    NodePath loose_xz_plane = create_plane_np(LPoint3f(0.5, 0, 0.5), LPoint3f(0, 0, 0), LColor(0, 1, 0, 1), AxisType::AT_xz);

    // Loose XY Plane
    NodePath loose_xy_plane = create_plane_np(LPoint3f(0.5, 0.5, 0.0), LPoint3f(0, -90, 0), LColor(0, 0, 1, 1), AxisType::AT_xy);

    // Plane representing "select all".
    NodePath select_all_plane = create_plane_np(LPoint3f(0), LPoint3f(0), LColor(1, 1, 0, 0), AxisType::AT_all);
    select_all_plane.set_billboard_point_eye(1);

    for (NodePath& planes : _axis_plane_nps) {
        planes.reparent_to(*_control_node);
    }

    // XYZ Line Segments
    for (int i = 0; i <= 2; i++) {
        // Set draw/color at i to 1.
        draw_point[i] = get_length();
        color[i] = 1;

        line_segs.move_to(0, 0, 0);
        line_segs.draw_to(draw_point);
        line_segs.set_thickness(get_thickness());

        line_seg_np = _control_node->attach_new_node(line_segs.create());
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

    // Some CollisionRay implementations (like FFD's clicker) may pick up things from
    // the object handles. For that reason, we set the collide mask to a dummy.
    NodePathCollection npc = _control_node->find_all_matches("**/+GeomNode");
    NodePath path;
    for (size_t i = 0; i < npc.get_num_paths(); i++) {
        path = npc.get_path(i);
        path.node()->set_into_collide_mask(CollideMask::bit(21));
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

    // Ignore if there is no mouse:
    if (!o_handle->_mouse_watcher->has_mouse()) {
        return AsyncTask::DS_again;
    }

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

    // Check for Axis Line Seg:
    for (NodePath& axis_np : o_handle->_axis_nps) {
        LColor color(0, 0, 0, 1);
        color[i] = 1.0;

        origin = LPoint3f(0);
        origin[i] = o_handle->get_length();

        // Reset color:
        axis_np.set_color_scale(color);

        point_2d = o_handle->convert_to_2d_space(axis_np, origin, proj_mat, mouse_xy);

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
        return AsyncTask::DS_again;
    }

    // Check for Plane Hover:
    i = 0;
    for (NodePath& plane_np : o_handle->_axis_plane_nps) {
        // For these..we can check the distance. It'll be a little off since it's circular, but we can accept that.
        // We can't do a traditional plane intersection unless there's somewhere in Plane to make its area finite.
        point_2d = o_handle->convert_to_2d_space(plane_np, LPoint3(0), proj_mat, mouse_xy);

        // Distance from the plane_np to the mouse.
        distance = (point_2d - mouse_xy).length();

        // Set original color:
        // planes are pushed into the vec in such a way that i represents the color comp that is set to 1.
        LColor color(0, 0, 0, 1);
        color[i] = 1;

        // The only exception to that is the last one. (select all plane is yellow):
        if (i == o_handle->_axis_plane_nps.size()-1) {
            color = LColor(1, 1, 0, 1);
        }

        // Distance check:
        if (distance <= o_handle->_PLANE_AXIS_D) {
            plane_np.set_color(1, 1, 1, 1);

            // Set "active":
            o_handle->_hover_line_np = plane_np;
        }
        else {
            plane_np.set_color(color);
        }
        i++;
    }

    o_handle->update_scale(proj_mat);

    return AsyncTask::DoneStatus::DS_cont;
}

/*
Updates the scale of the object handle based on the camera.
*/
void ObjectHandles::update_scale(LMatrix4 &proj_mat) {
    // Ignore if we're not active.
    if (!_active) {
        return;
    }

    LPoint3f point_3d;
    point_3d = _camera_np.get_relative_point(_np, point_3d);
    LVecBase4f point_cam = proj_mat.xform(LVecBase4f(point_3d[0], point_3d[1], point_3d[2], 1.0));
    _control_node->set_scale(point_cam[3] * 0.1);
}

/*
Object 3D -> 2D Space conversion.
*/
LPoint2f ObjectHandles::convert_to_2d_space(NodePath& np, LPoint3f& origin, LMatrix4& proj_mat, LPoint2f& mouse_xy) {
    LPoint3f point_3d;

    // Object -> Camera Space
    point_3d = _camera_np.get_relative_point(np, origin);

    // From: tarsierpi on p3d forums:
    LVecBase4f point_cam_2d = proj_mat.xform(LVecBase4f(point_3d[0], point_3d[1], point_3d[2], 1.0));

    // point_cam_2d[3] provides us the forward "distance" from the object to the camera. We use this to scale our other 2 axis.
    double point_cam_2d_3 = 1.0 / point_cam_2d[3];

    return LPoint2f(
        point_cam_2d[0] * point_cam_2d_3,
        point_cam_2d[1] * point_cam_2d_3
    );
}

AsyncTask::DoneStatus ObjectHandles::mouse_drag_task(GenericAsyncTask* task, void* args) {
    // Args is ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)(args);

    // We can get the axis of our handle via a tag it has:
    int axis = atoi(o_handle->_active_line_np.get_net_tag("axis").c_str());

    // We modify the plane's normal to determine what axis we're intersecting.
    // This'll be useful for not only single axis movements, but 2-axis ones too.
    LVector3f normal;

    switch (axis) {
        case AxisType::AT_z: case AxisType::AT_xz:
            normal = LVector3f(0, 1, 0);
            break;
        case AxisType::AT_yz:
            normal = LVector3f(1, 0, 0);
            break;
        default:
            normal = LVector3f(0, 0, 1);
    }

    // Get world space of the object handle:
    LPlanef plane = LPlanef(normal, LPoint3f(0, 0, 0));
    LPoint3f _near, _far, pos3d;

    NodePath parent = o_handle->get_parent();

    // https://discourse.panda3d.org/t/super-fast-mouse-ray-collisions-with-ground-plane/5022
    LPoint3f pos = o_handle->_np_obj_node->get_pos(parent);

    o_handle->_camera->get_lens()->extrude(o_handle->_mouse_watcher->get_mouse(), _near, _far);

    // Check for intersection:
    plane.intersects_line(pos3d,
        parent.get_relative_point(o_handle->_camera_np, _near),
        parent.get_relative_vector(o_handle->_camera_np, _far));

    // previous_pos3d is pos3d if its all zeros.
    if (o_handle->previous_pos3d.almost_equal(LPoint3f::zero())) {
        o_handle->previous_pos3d = pos3d;
    }

    // We don't actually want to set the o_handle's pos to the intersection one.
    // This'll cause the node to jump to the exact pos. Instead, we move pos relative to pos3d.
    // For this, it's just the delta.
    LPoint3f delta_pos3d = pos3d - o_handle->previous_pos3d;

    // Reset previous_pos3d.
    o_handle->previous_pos3d = pos3d;

    switch (axis) {
        case AxisType::AT_all:
            pos += delta_pos3d;
            break;
        case AxisType::AT_xy:
            pos[0] += delta_pos3d[0];
            pos[1] += delta_pos3d[1];
            break;
        case AxisType::AT_xz:
            pos[0] += delta_pos3d[0];
            pos[2] += delta_pos3d[2];
            break;
        case AxisType::AT_yz:
            pos[1] += delta_pos3d[1];
            pos[2] += delta_pos3d[2];
            break;
        default:
            pos[axis] += delta_pos3d[axis];
    }

    o_handle->set_pos(pos);

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
    
    // Reset previous_pos3d:
    o_handle->previous_pos3d = LPoint3f::zero();
}

void ObjectHandles::handle_click(const Event* event, void* args) {
    // Args is ObjectHandles instance.
    ObjectHandles* o_handle = (ObjectHandles*)(args);

    // Ignore if there is no mouse:
    if (!o_handle->_mouse_watcher->has_mouse()) {
        return;
    }

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
    PT(AsyncTask) _drag_task = new GenericAsyncTask(o_handle->DRAG_TASK_NAME, mouse_drag_task, o_handle);
    o_handle->_drag_task = _drag_task;
    task_mgr->add(_drag_task);

    // Ignore camera movement:
    o_handle->disable_camera_movement();

}

void ObjectHandles::setup_mouse_watcher() {
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    
    // Mouse task for highlighting (dragging is separate for readability purposes):
    AsyncTask* task = new GenericAsyncTask(MOUSE_TASK_NAME, mouse_task, this);
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

void ObjectHandles::set_active(bool active) {
    _active = active;

    if (!active) {
        hide();
    }
    else {
        show();
    }
}

bool ObjectHandles::is_active() {
    return _active;
}

/*
Adds NodePath to be tracked with object handles.
Will not add if given np is empty.
*/
void ObjectHandles::add_node_path(NodePath &np) {
    if (np.is_empty()) {
        return;
    }

    // Reparent:
    reparent_to(np.get_parent().get_parent());

    // Keep track of who is selected:
    _nps[np.get_parent()].push_back(np);

    // Next, reparent them to the selected NP holder:
    np.wrt_reparent_to(*_np_obj_node);

    // Reposition our handles to the center of all of our NPs.
    PT(BoundingVolume) b_volume = _np_obj_node->get_bounds();
    CPT(BoundingSphere) b_sphere = b_volume->as_bounding_sphere();

    _control_node->set_pos(b_sphere->get_center());
    rebuild();
}

/*
Removes all tracked NodePaths.
*/
void ObjectHandles::clear_node_paths() {
    // Reparent our children back to their original parent nodes.
    for (pmap<NodePath, pvector<NodePath>>::iterator it = _nps.begin(); it != _nps.end(); it++) {
        for (NodePath& child : it->second) {
            child.wrt_reparent_to(it->first);
        }
    }

    // Then clear our map.
    _nps.clear();
}
