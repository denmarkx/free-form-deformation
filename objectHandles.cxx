#include "objectHandles.h"

ObjectHandles::ObjectHandles(NodePath np, NodePath mouse_np, NodePath camera_np, Camera *camera, NodePath aspect2d, NodePath render2d) : NodePath("ObjectHandles") {
    _np = np;
    _camera_np = camera_np;
    _aspect2d = aspect2d;
    _render2d = _render2d;
    _camera = camera;

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

        // Otherwise, we're officially within range.
        axis_np.set_color_scale(1, 1, 0, 1);
        i++;
    }
    return AsyncTask::DoneStatus::DS_cont;
}

void ObjectHandles::setup_mouse_watcher() {
    AsyncTaskManager* task_mgr = AsyncTaskManager::get_global_ptr();
    
    AsyncTask* task = new GenericAsyncTask("ObjectHandles_MouseTask", mouse_task, this);
    task_mgr->add(task);
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
