#include "geomVertexData.h"
#include "nodePath.h"
#include "loader.h"

class Lattice : public NodePath {
public:
    Lattice(NodePath np);
    void create_control_points();
    void calculate_lattice_vec();
    void set_edge_spans(int size_x, int size_y, int size_z);

private:
    void create_point(LPoint3f point);
    void rebuild();

    int _plane_spans[3] = { 1, 3, 2 };

    Loader *_loader = Loader::get_global_ptr();

    pvector<LPoint3f> _control_points;
    LVector3f _lattice_vec; // STU
    LPoint3f _x0, _x1;

    NodePath _np;
    NodePath _control_point_root;
};
