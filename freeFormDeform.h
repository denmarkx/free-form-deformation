#include "nodePath.h"
#include "geomNode.h"
#include "lpoint3.h"
#include "lvector3.h"
#include "geomVertexRewriter.h"
#include "collisionRay.h"
#include "mouseWatcher.h"
#include "collisionTraverser.h"
#include "collisionHandlerQueue.h"
#include "collisionNode.h"
#include "pandaFramework.h"
#include "mouseWatcher.h"
#include "camera.h"

#include "lattice.h"

typedef struct ClickerArgs;

class FreeFormDeform {
public:
    FreeFormDeform(NodePath np, NodePath render);
    ~FreeFormDeform();

    void destroy();

    void set_edge_spans(int size_x, int size_y, int size_z);
    void update_vertices();

    void setup_clicker(WindowFramework& window);

private:
    void process_node();
    void transform_vertex(GeomVertexData* data);

    static void handle_click(const Event* event, void* args);
    static AsyncTask::DoneStatus drag_task(GenericAsyncTask* task, void* data);


    int factorial(int n);
    int binomial_coeff(int n, int k);
    double bernstein(double v, double n, double x);
    LPoint3f point_at_axis(double axis_value, LPoint3f point, LVector3f vector, int axis);

    NodePath _np;
    NodePath _render;
    NodePath _picker_node;

    Lattice* _lattice;

    LVector3f deform_vertex(double s, double t, double u);

    CollisionTraverser* _traverser;

    PT(CollisionHandlerQueue) _handler_queue;
    PT(CollisionRay) _collision_ray;

    AsyncTaskManager *_task_mgr = AsyncTaskManager::get_global_ptr();
    PT(AsyncTask) _clicker_task;

    ClickerArgs* _c_args;

    pvector<pvector<LPoint3f>> _default_vertex_ws_os; // Default vertex, default object space vertex.

    pvector<PT(GeomNode)> _geom_nodes;
    pvector<int> _selected_points;
};

typedef struct ClickerArgs {
    MouseWatcher* mouse;
    WindowFramework* window;
    FreeFormDeform* ffd;
} ClickerArgs;
