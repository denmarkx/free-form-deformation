#include "nodePath.h"
#include "geomNode.h"
#include "lpoint3.h"
#include "lvector3.h"
#include "lattice.h"

class FreeFormDeform {
public:
    FreeFormDeform(NodePath np);
    void set_edge_spans(int size_x, int size_y, int size_z);
    void set_edge_spans_DEBUG();
private:
    void process_node();

    NodePath _np;
    Lattice* _lattice;

    pvector<Geom*> _geoms;
    pvector<GeomVertexData*> _vertex_data;
};

