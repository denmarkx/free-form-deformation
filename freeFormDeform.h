#include "nodePath.h"
#include "geomNode.h"
#include "lpoint3.h"
#include "lvector3.h"
#include "lattice.h"
#include "geomVertexRewriter.h"
#include <math.h>

class FreeFormDeform {
public:
    FreeFormDeform(NodePath np, NodePath render);
    void set_edge_spans(int size_x, int size_y, int size_z);
    void update_vertices();

private:
    void process_node();
    void transform_vertex(GeomVertexData* data);

    LVector3f deform_vertex(double s, double t, double u);

    int factorial(int n);
    int binomial_coeff(int n, int k);
    int bernstein(int v, int n, int x);

    NodePath _np;
    NodePath _render;
    Lattice* _lattice;

    pvector<GeomNode*> _geom_nodes;
};

