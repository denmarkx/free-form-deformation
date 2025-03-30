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

    double get_displacement(LPoint2f& pointA, LPoint2f& pointB);

    LPoint2f get_screen_space_origin(LPoint3f origin, LMatrix4 &proj_mat);

    void rebuild();
    void setup_mouse_watcher();

    double _length = 0.5;
    double _thickness = 1.0;

    NodePath _np, _camera_np, _aspect2d, _render2d;
    
    Camera *_camera;
    PT(MouseWatcher) _mouse_watcher;

    pvector<NodePath> _axis_nps;
};