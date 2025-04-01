#include "lattice.h"

Lattice::Lattice(NodePath np) : NodePath("FFD_Lattice") {
    _np = np;
    rebuild();
}

Lattice::~Lattice() {
    remove_node();
}

void Lattice::rebuild() {
    calculate_lattice_vec();
    create_control_points();
}

void Lattice::create_control_points() {
    reset_control_points();

    LPoint3f point;

    for (size_t i = 0; i <= _plane_spans[0]; i++) {
        for (size_t j = 0; j <= _plane_spans[1]; j++) {
            for (size_t k = 0; k <= _plane_spans[2]; k++) {
                point = LPoint3f(
                    _x0[0] + ((double)i / _plane_spans[0]) * _lattice_vecs[0][0],
                    _x0[1] + ((double)k / _plane_spans[2]) * _lattice_vecs[2][1],
                    _x0[2] + ((double)j / _plane_spans[1]) * _lattice_vecs[1][2]
                );

                create_point(point);
            }
        }
    }
}

NodePath& Lattice::get_control_point(int index) {
    return _control_points[index];
}

void Lattice::set_control_point_pos(LPoint3f pos, int index) {
    NodePath c_point = _control_points[index];
    c_point.set_pos(pos);
}

void Lattice::reset_control_points() {
    for (NodePath& c_point : _control_points) {
        c_point.remove_node();
    }
    _control_points.clear();
}

LPoint3f Lattice::get_x0() {
    return this->_x0;
}

pvector<LVector3f> Lattice::get_lattice_vecs() {
    return this->_lattice_vecs;
}

LPoint3f Lattice::get_control_point_pos(int i, const NodePath& other) {
    return _control_points[i].get_pos(other);
}

void Lattice::create_point(LPoint3f point) {
    PT(PandaNode) p_node = _loader->load_sync("misc/sphere");
    NodePath c_point = this->attach_new_node(p_node);
    c_point.set_pos(point);
    c_point.set_scale(0.05);
    c_point.set_color(1, 0, 1, 1);
    c_point.set_tag("control_point", std::to_string(_control_points.size()));
    _control_points.push_back(c_point);
}

void Lattice::set_edge_spans(int size_x, int size_y, int size_z) {
    _plane_spans.clear();
    _plane_spans.push_back(size_x);
    _plane_spans.push_back(size_y);
    _plane_spans.push_back(size_z);
    rebuild();
}

void Lattice::calculate_lattice_vec() {
    _lattice_vecs.clear();
    _np.calc_tight_bounds(_x0, _x1);
    double size_s = _x1[0] - _x0[0];
    double size_t = _x1[2] - _x0[2];
    double size_u = _x1[1] - _x0[1];

    LVector3f s = LVector3f(size_s, 0, 0);
    LVector3f t = LVector3f(0, 0, size_t);
    LVector3f u = LVector3f(0, size_u, 0);
    
    _lattice_vecs.push_back(s);
    _lattice_vecs.push_back(t);
    _lattice_vecs.push_back(u);
}

pvector<int> Lattice::get_edge_spans() {
    return _plane_spans;
}

