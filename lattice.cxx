#include "lattice.h"
#include "lineSegs_ext.h"

void Lattice::rebuild() {
    CPT(BoundingSphere) b_sphere = _np.get_bounds()->as_bounding_sphere();
    const double radius = b_sphere->get_radius();
    
    calculate_lattice_vec();
    create_control_points(radius);
    create_edges();

    watch_node_path(*this, 2);
}

void Lattice::create_control_points(const double radius) {
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
                create_point(point, radius, i, j, k);
            }
        }
    }

    _dom->register_object(*this);
}

void Lattice::push_point_edge(int index) {
    point_to_edge_vertex[index].push_back(num_segments);
}

void Lattice::push_point_relationship(int index, int adjacent_index) {
    // Add the adjacent index to the regular vector.
    point_map[index].push_back(adjacent_index);

    pvector<int> index_v = point_map[index];

    // Check if we have anything in our holding vector:
    if (point_map_future[index].size() > 0) {
        // Yes, add them all to point_map if they aren't in it.
        int future_index;
        for (size_t i = 0; i < point_map_future[index].size(); i++) {
            future_index = point_map_future[index][i];
            index_v = point_map[index];
  
            // Ensure the future index is still not in the future:
            if (future_index > index) {
                break;
            }

            if (std::find(index_v.begin(), index_v.end(), future_index) == index_v.end()) {
                point_map[index].push_back(future_index);
            }
        }
    }

    // Ignore if not found:
    //if (point_map.find(adjacent_index) == point_map.end()) {
    if (point_map.count(adjacent_index) == 0) {
        // We could either do an iteration each function call, but we'll
        // just hold this for later.
        point_map_future[adjacent_index].push_back(index);
    }

    // Check if index is in adjacent_index's vector:
    pvector<int> adj_index_v = point_map[adjacent_index];

    // Not in here..let's add.
    if (std::find(adj_index_v.begin(), adj_index_v.end(), index) == adj_index_v.end()) {
        point_map[adjacent_index].push_back(index);
    }
}

void Lattice::create_edges() {
    reset_edges();

    _edges = LineSegs("lattice_edges");

    // TODO: there's a lot of dead vertices here because i didn't realize at the time that
    // both move and draw to pushes back on segs. an optimal approach would be to try and 
    // connect multiple line segments to a given vertex.

    int l = _plane_spans[2];

    // Get l*n and m*n (+1 account for 0 start).
    int l_n = (_plane_spans[0] + 1) * (_plane_spans[2] + 1);
    int m_n = (_plane_spans[1] + 1) * (_plane_spans[2] + 1);

    // Get the total (which is l_n * (m+1)).
    int total = l_n * (_plane_spans[1] + 1);

    // Control Point:
    NodePath &pnt = NodePath();

    bool go_down = false;

    // This next for loop is divided into three drawing scenarios for edges:
    // Forward (+y)
    // Right (+x)
    // Down (-z)

    // Pre-traverse total (ugh):
    pvector<int> empty_vec;
    for (size_t i = 0; i < total; i++) {
        point_map_future[i] = empty_vec;
        point_map[i] = empty_vec;
        point_to_edge_vertex[i] = empty_vec;
    }

    for (size_t i = 0; i < total; i++) {
        // Get current control point:
        pnt = _control_points[i];

        // Place our vertex at the current point:
        _edges.move_to(pnt.get_pos());
        num_segments++;

        // Add to vector:
        push_point_edge(i);

        // We love ourselves:
        push_point_relationship(i, i);

        // Forward:
        // +y is sequentially sorted, so there isn't much thought that goes into it.
        // We just want to make sure we don't go forward at the very end.
        // Check if (i+1) is a factor of (l+1). If it is, then we DON'T go forward.
        if ((i + 1) % (l + 1) != 0) {
            // We move forward by 1.
            pnt = _control_points[i + 1];
            _edges.draw_to(pnt.get_pos());
            num_segments++;
            point_to_edge_vertex[i + 1].push_back(num_segments);

            // Add to vector:
            push_point_relationship(i, i + 1);
        }

        // Right:
        // From the current vertex, we move +x by (m+1)*(n+1).
        // We can check for an illegal move by comparing the number of points.
        if (i + m_n < _control_points.size()) {
            // Move back to where we were..if we moved..
            pnt = _control_points[i];
            if (_edges.get_current_position() != pnt.get_pos()) {
                _edges.move_to(pnt.get_pos());
                num_segments++;
                push_point_edge(i);
            }

            // i -> i + (m+1)*(n+1):
            pnt = _control_points[i + m_n];
            _edges.draw_to(pnt.get_pos());
            num_segments++;

            point_to_edge_vertex[i+m_n].push_back(num_segments);

            push_point_relationship(i, i + m_n);
        }

        // Down:
        // We check if i, i-1, or i-2 % 9 = 0
        // For this one, we go -z. To validate for a legal move,
        // we check if n, n-1, or n-2 (where n=i-j) is a factor of m_n.
        // If any of them ARE a factor, we do NOT go down.
        go_down = false;

        // ..ensure we've above l.
        if (i < l) {
            continue;
        }

        // n,n-1,n-2 check:
        for (size_t j = 0; j <= l; j++) {
            go_down = true;
            if ((i - j) % m_n == 0) {
                go_down = false;
                break;
            }
        }

        // This is the end so we can continue on here:
        if (!go_down) {
            continue;
        }


        // Move back to where we were..if we moved..
        pnt = _control_points[i];
        if (_edges.get_current_position() != pnt.get_pos()) {
            _edges.move_to(pnt.get_pos());
            num_segments++;
            push_point_edge(i);
        }

        // Draw:
        pnt = _control_points[i - (l + 1)];
        _edges.draw_to(pnt.get_pos());
        num_segments++;

        point_to_edge_vertex[i - (l + 1)].push_back(num_segments);

        push_point_relationship(i, i - (l+1));

    }

    // Attach to self.
    _edgesNp = attach_new_node(_edges.create());
    _edgesNp.set_tag("LINE_SEG_TAG", "");

    // Util for picking LineSegs:
    LINESEGS_EXT::process_lines(_edges, _edgesNp);
}

/*
Updates the adjacent edges to match the given control point.
*/

void Lattice::update_edges(int index) {
    // Update the edges:
    pvector<int> &adjacent_points = point_map[index];
    for (int i = 0; i < adjacent_points.size(); i++) {
        for (int j = 0; j < point_to_edge_vertex[adjacent_points[i]].size(); j++) {
            if (adjacent_points[i] == index) {
                _edges.set_vertex(point_to_edge_vertex[index][j], get_control_point_pos(index, _edgesNp));
            }
        }
    }
    
    // Update the collision capsules:
    LINESEGS_EXT::update_lines(_edges, _edgesNp);
}

void Lattice::reset_edges() {
    _edgesNp.remove_node();
    _edges.reset();
    point_map_future.clear();
    point_map.clear();
    point_to_edge_vertex.clear();
    num_segments = -1;
}

void Lattice::reset_control_points() {
    for (NodePath& c_point : _control_points) {
        c_point.remove_node();
    }
    _control_points.clear();
    _point_ijk_map.clear();
}

void Lattice::create_point(LPoint3f point, const double radius, int i, int j, int k) {
    PT(PandaNode) p_node = _loader->load_sync("misc/sphere");
    NodePath c_point = this->attach_new_node(p_node);
    c_point.set_pos(point);
    c_point.set_scale(0.05 * radius);
    c_point.set_color(1, 0, 1, 1);
    c_point.set_tag("control_point", std::to_string(_control_points.size()));
    _control_points.push_back(c_point);

    // Place in point_ijk_map:
    _point_ijk_map[_control_points.size() - 1] = std::vector<int>{ i, j, k };
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
    if (!initial_bounds_capture) {
        _np.calc_tight_bounds(_x0, _x1);
        initial_bounds_capture = true;
    }
    else {
        _edgesNp.calc_tight_bounds(_x0, _x1, _np.get_top());
        _edgesNp.show_tight_bounds();
    }

    LPoint3f delta = _edgesNp.get_pos(_np.get_top()) - _edge_pos;

    for (NodePath& cp : _control_points) {
        cp.set_pos(cp.get_pos() + delta);
    }

    _edge_pos = _edgesNp.get_pos(_np.get_top());

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

/*
Returns boolean representing if the given point is
between the Lattice's bounds (x_min, x_max).
*/
bool Lattice::point_in_range(LPoint3f &point) {
    LVector3f x_min = get_x0();
    LVector3f x_max = get_x1();

    for (size_t i = 0; i < 3; i++) {
        // There's a precision issue because tight bounds literally means tight bounds.
        // ..hence the reason for an epsilon.
        if (x_min[i]-0.00001 > point[i] || x_max[i]+0.00001 < point[i]) {
            return false;
        }
    }
    return true;
}

/*
* Inherits DraggableObject::select, updates internal tracking
* of selected control point indices.
*/
void Lattice::select(NodePath& np) {
    DraggableObject::select(np);

    // Ignore if we're not a control point:
    if (!np.has_net_tag("control_point")) {
        return;
    }

    // Push to _selected_control_points:
    _selected_control_points.push_back(std::stoi(np.get_net_tag("control_point")));
}
/*
* Inherits DraggableObject::deselect, updates internal tracking
* of deselected control point indices.
*/
void Lattice::deselect(NodePath& np) {
    DraggableObject::deselect(np);

    // Ignore if we're not a control point:
    if (!np.has_net_tag("control_point")) {
        return;
    }

    // Remove:
    _selected_control_points.erase(
        std::remove(
            _selected_control_points.begin(),
            _selected_control_points.end(),
            std::stoi(np.get_net_tag("control_point"))
        ),
        _selected_control_points.end()
    );
}

