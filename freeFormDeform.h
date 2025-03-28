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

class FreeFormDeform {
public:
    FreeFormDeform(NodePath np, NodePath render);
    void set_edge_spans(int size_x, int size_y, int size_z);
    void update_vertices();

    void setup_clicker(Camera *camera, PandaFramework& framework, WindowFramework& window);

private:
    void process_node();
    void transform_vertex(GeomVertexData* data);
    static void handle_click(const Event* event, void* args);

    LVector3f deform_vertex(double s, double t, double u);

    int factorial(int n);
    int binomial_coeff(int n, int k);
    int bernstein(int v, int n, int x);

    NodePath _np;
    NodePath _render;
    Lattice* _lattice;
    CollisionRay* _collision_ray;
    CollisionTraverser* _traverser;
    CollisionHandlerQueue* _handler_queue;

    pvector<GeomNode*> _geom_nodes;
};

typedef struct ClickerArgs {
    MouseWatcher* mouse;
    WindowFramework *window;
    FreeFormDeform* ffd;
} ClickerArgs;

