#include "nodePath.h"
#include "genericAsyncTask.h"
#include "mouseWatcher.h"
#include "lineSegs.h"
#include "asyncTaskManager.h"
#include "camera.h"

class ObjectHandles : public NodePath {
public:
    ObjectHandles(NodePath np, NodePath _mouse_np, NodePath camera_np, Camera *camera, NodePath aspect2d, NodePath render2d);

    void set_length(double length);
    double get_length();

    void set_thickness(double thickness);
    double get_thickness();
    
private:
    static AsyncTask::DoneStatus mouse_task(GenericAsyncTask* task, void* args);
    static AsyncTask::DoneStatus mouse_drag_task(GenericAsyncTask* task, void* args);
    static void handle_click(const Event* event, void* args);
    static void handle_drag_done(const Event* event, void* args);

    LPoint2f get_screen_space_origin(LPoint3f origin, LMatrix4 &proj_mat);

    void rebuild();
    void setup_mouse_watcher();

    double _length = 0.5;
    double _thickness = 1.0;

    NodePath _np, _camera_np, _aspect2d, _render2d;
    NodePath _hover_line_np, _active_line_np;

    Camera *_camera;
    PT(MouseWatcher) _mouse_watcher;

    pvector<NodePath> _axis_nps;

    AsyncTask *_drag_task;
};