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
#include "objectHandles.h"

typedef struct ClickerArgs;

class FreeFormDeform {
public:
    FreeFormDeform(NodePath np, NodePath render);
    ~FreeFormDeform();

    void set_edge_spans(int size_x, int size_y, int size_z);
    void update_vertices();

    void setup_clicker(WindowFramework& window);

private:
    void process_node();
    void transform_vertex(GeomVertexData* data, GeomNode* geom, int index);

    static void handle_click(const Event* event, void* args);
    static AsyncTask::DoneStatus drag_task(GenericAsyncTask* task, void* data);


    double factorial(double n);
    int binomial_coeff(int n, int k);
    double bernstein(double v, double n, double x);
    LPoint3f point_at_axis(double axis_value, LPoint3f point, LVector3f vector, int axis);
    bool is_influenced(int index, double s, double t, double u);
    int get_point_index(int i, int j, int k);
    std::vector<int> get_ijk(int index);

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

    pvector<PT(GeomNode)> _geom_nodes;

    // GeomNode -> {c_point : [vertex..]}
    typedef pmap<int, pvector<int>> __internal_vertices;
    pmap<PT(GeomNode), __internal_vertices> _influenced_vertices;

    // GeomNode -> [[default_vertex_world_space, default_vertex_object_space]]
    typedef pvector<pvector<LPoint3f>> __internal_default_vertices_pos;
    pmap<PT(GeomNode), __internal_default_vertices_pos> _default_vertex_ws_os; // Default vertex, default object space vertex.

    ObjectHandles* _object_handles;
    pvector<int> _selected_points;
};

typedef struct ClickerArgs {
    MouseWatcher* mouse;
    WindowFramework* window;
    FreeFormDeform* ffd;
} ClickerArgs;
