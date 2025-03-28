#include "lattice.h"

Lattice::Lattice(NodePath np) : NodePath("FFD_Lattice") {
    _np = np;

    this->reparent_to(np);
    _control_point_root = this->attach_new_node("_control_point_root");

    rebuild();
}

void Lattice::rebuild() {
    _control_point_root.remove_node();
    _control_point_root = this->attach_new_node("_control_point_root");
    _control_points.clear();

    calculate_lattice_vec();
    create_control_points();
}

void Lattice::create_control_points() {
    LPoint3f point;
    _lattice_vec.output(nout);
    for (size_t i = 0; i <= _plane_spans[0]; i++) {
        for (size_t j = 0; j <= _plane_spans[1]; j++) {
            for (size_t k = 0; k <= _plane_spans[2]; k++) {
                point = LPoint3f(
                    ((double)i / _plane_spans[0]) * _lattice_vec[0],
                    ((double)j / _plane_spans[1]) * _lattice_vec[1],
                    ((double)k / _plane_spans[2]) * _lattice_vec[2]
                );
                create_point(point);
                _control_points.push_back(point);
            }
        }
    }
}

void Lattice::create_point(LPoint3f point) {
    PT(PandaNode) p_node = _loader->load_sync("misc/sphere");
    NodePath c_point = _control_point_root.attach_new_node(p_node);
    c_point.set_pos(point);
    c_point.set_scale(0.05);
    c_point.set_color(1, 0, 1, 1);
}

void Lattice::set_edge_spans(int size_x, int size_y, int size_z) {
    _plane_spans[0] = size_x;
    _plane_spans[1] = size_y;
    _plane_spans[2] = size_z;
    rebuild();
}

void Lattice::calculate_lattice_vec() {
    _np.calc_tight_bounds(_x0, _x1);
    double size = 0.0;
    for (size_t i = 0; i < 3; i++) {
        size = _x1[i] - _x0[i];
        _lattice_vec[i] = size;
    }
}
