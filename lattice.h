#include "geomVertexData.h"
#include "nodePath.h"
#include "loader.h"
#include "boundingSphere.h"
#include "lineSegs.h"
#include "textNode.h"

class Lattice : public NodePath {
public:
    Lattice(NodePath np);
    ~Lattice();

    void create_control_points(const double radius);
    void calculate_lattice_vec();

    void set_edge_spans(int size_x, int size_y, int size_z);
    void set_control_point_pos(LPoint3f pos, int index);

    NodePath& get_control_point(int index);

    pvector<int> get_edge_spans();
    pvector<LVector3f> get_lattice_vecs();
    LPoint3f get_control_point_pos(int i, const NodePath& other);
    LPoint3f get_x0();
    LPoint3f get_x1();
    int get_num_control_points();

    bool point_in_range(LPoint3f& point);

private:
    void create_point(LPoint3f point, const double radius, int i, int j, int k);
    void create_edges();
    void reset_control_points();
    void reset_edges();
    void rebuild();
    void push_point_edge(int index);
    void push_point_relationship(int index, int adjacent_index);

    Loader *_loader = Loader::get_global_ptr();

    pvector<NodePath> _control_points; // P(ijk)
    pvector<LVector3f> _lattice_vecs; // STU
    pvector<int> _plane_spans = { 2, 3, 2 }; // lnm

    LPoint3f _x0, _x1;

    NodePath _np;

    LineSegs _edges;

    // control point index -> line segment index
    pmap<int, pvector<int>> point_to_edge_vertex;

    // control point index -> control point index
    pmap<int, pvector<int>> point_map;

    std::map<int, std::vector<int>> point_map_future;

    int num_segments = -1;
};
