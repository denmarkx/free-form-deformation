#ifndef OBJECTHANDLES_H
#define OBJECTHANDLES_H

#include "nodePath.h"
#include "genericAsyncTask.h"
#include "mouseWatcher.h"
#include "lineSegs.h"
#include "asyncTaskManager.h"
#include "camera.h"
#include "trackball.h"
#include "transform2sg.h"
#include "mouseButton.h"
#include "plane.h"
#include "cardMaker.h"
#include "planeNode.h"
#include "loader.h"

#include "nearly_zero.h"
class ObjectHandles : public NodePath {
public:
    ObjectHandles(NodePath &np, NodePath &parent, NodePath _mouse_np, NodePath camera_np, Camera *camera, NodePath aspect2d, NodePath render2d);
    void cleanup();

    void set_length(double length);
    double get_length();

    void set_thickness(double thickness);
    double get_thickness();

    void set_active(bool active);
    bool is_active();

    void set_node_path(NodePath &np, double scale_mult);
    NodePath get_node_path();


private:
    enum AxisType {
        AT_x,
        AT_y,
        AT_z,
        AT_xy,
        AT_xz,
        AT_yz,
        AT_all,
    };

private:
    static AsyncTask::DoneStatus mouse_task(GenericAsyncTask* task, void* args);
    static AsyncTask::DoneStatus mouse_drag_task(GenericAsyncTask* task, void* args);
    static void handle_click(const Event* event, void* args);
    static void handle_drag_done(const Event* event, void* args);

    LPoint2f get_screen_space_origin(LPoint3f origin, LMatrix4 &proj_mat);
    LPoint2f convert_to_2d_space(NodePath& np, LPoint3f& origin, LMatrix4& proj_mat, LPoint2f& mouse_xy);
    NodePath create_plane_np(LPoint3f pos, LPoint3f hpr, LColor color, AxisType axis_type);

    void rebuild();
    void setup_mouse_watcher();
    void disable_camera_movement();
    void enable_camera_movement();

private:
    double _length = 0.5;
    double _thickness = 1.0;
   
    const double _PLANE_AXIS_D = 0.07;

    bool _active;

    NodePath _np;
    NodePath _np_parent;
    NodePath _camera_np;
    NodePath _mouse_np;
    NodePath _trackball_np;
    NodePath _hover_line_np;
    NodePath _active_line_np;

    PT(Trackball) _trackball;
    PT(MouseWatcher) _mouse_watcher;
    PT(Transform2SG) _trackball2Cam;

    Camera* _camera;
    LMatrix4f _cam_mat;

    PandaNode* _trackball_node;

    AsyncTask *_drag_task;

    pvector<NodePath> _axis_nps;
    pvector<NodePath> _axis_plane_nps;
};
#endif
