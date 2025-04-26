#include "freeFormDeform.h"
#include <unordered_set>

/*
* Initializer for FreeFormDeform. NodePath is the object wanting to deform.
* Automatically contructs the Lattice and calls hook_drag_event on it
* with an internal FFD_DRAG_EVENT event.
*/
FreeFormDeform::FreeFormDeform(NodePath np, NodePath render) {
    _np = np;
    _geom_node_collection = _np.find_all_matches("**/+GeomNode");
    _top_node = _np.get_top();
    _render = render;

    _lattice = new Lattice(_np);
    _lattice->reparent_to(_render);
    _lattice->hook_drag_event("FFD_DRAG_EVENT", handle_drag, this);

    populate_lookup_table();
    process_node();
}

/*
* See also: freeFormDeform.I (berstein)
* Computes the binomial_coeff between two variables:
*   n: every edge span
*   v: range: [0, n]
*/
void FreeFormDeform::populate_lookup_table() {
    _v_n_comb_table.clear();
    for (int& span : _lattice->get_edge_spans()) { // ijk range = 0->span
        std::vector<int> table;
        for (int i = 0; i <= span; i++) {
            table.push_back(binomial_coeff(span, i));
        }
        _v_n_comb_table.push_back(table);
    }
}

/*
* The drag event for which we hook Lattice onto.
*
* If you wish to move the main NodePath (given in initializer),
* you will have to make it a DraggableObject and hook a new drag event
* to this callback function.
*/
void FreeFormDeform::handle_drag(const Event* e, void* args) {
    FreeFormDeform* ffd = (FreeFormDeform*)args;
    ffd->process_node();
    ffd->update_vertices(e->get_name() != "FFD_DRAG_EVENT");
}

/*
* Simple recursive implementation for factorial.
*/
double FreeFormDeform::factorial(double n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}

/*
* Returns the actual index of the control point given i, j, k.
*/
int FreeFormDeform::get_point_index(int i, int j, int k) {
    std::vector<int>& spans = _lattice->get_edge_spans();
    return i * (spans[2] + 1) * (spans[2] + 1) + j * (spans[1] + 1) + k;
}


/*
* Returns true/false if the given control point influences the given vertex (stu).
* The berstein polynomial (for all ijk and spans and stu), will return 0 if there's no influence.
*/
bool FreeFormDeform::is_influenced(int index, LVector3f stu) {
    std::vector<int>& spans = _lattice->get_edge_spans();
    std::vector<int> &ijk = _lattice->get_ijk(index);
    return bernstein(ijk[0], 0, spans[0], stu[0]) * bernstein(ijk[1], 1, spans[1], stu[1]) * bernstein(ijk[2], 2, spans[2], stu[2]);
}

/*
* Resets all vertices of the given <data> that are not influenced by any control point.
*/
void FreeFormDeform::reset_vertices(GeomVertexData* data, GeomNode* geom_node, std::vector<int>& indices) {
    if (_non_influenced_vertex.find(geom_node) == _non_influenced_vertex.end()) {
        return;
    }

    GeomVertexWriter rewriter(data, "vertex");
    LPoint3f default_vertex_pos;
    
    // Now, we want to iterate through vertices that are NOT affected at all.
    int vertex_count = 0;

    // Iterate and set to the previous default space we got from calling process_node for the first time.
    for (size_t i = 0; i < _non_influenced_vertex[geom_node].size(); i++) {
        int t = _non_influenced_vertex[geom_node][i];
        rewriter.set_row(t);
        default_vertex_pos = _default_vertex_ws_os[geom_node][t][0];
        rewriter.set_data3f(default_vertex_pos);
    }
}

/*
* Deforms all vertices within <data> that are influenced without regard for control point information.
*/
void FreeFormDeform::transform_all_influenced(GeomVertexData* data, GeomNode* geom_node) {
    GeomVertexWriter rewriter(data, "vertex");
    LPoint3f default_vertex, default_object_space; // (stu)
    LVector3f x_ffd;

    pmap<int, pvector<int>> influence_map = _influenced_vertices[geom_node]; // key is ctrl point.
    pvector<int> vertices;
    pvector<LPoint3f> default_vertex_pos;

    std::unordered_set<int> _vertices2;

    // Our vertex can be controlled by multiple points.
    // For this reason, we need to remove any duplicates
    // so we're not doing unnecessary processing.
    int vertex = 0;
    for (int i = 0; i < influence_map.size(); i++) {
        vertices = influence_map[i];
        for (int j = 0; j < vertices.size(); j++) {
            vertex = vertices[j];
            _vertices2.insert(vertex);
        }
    }

    // Iterate through duplicates and deform.
    for (const int& vertex : _vertices2) {
        rewriter.set_row(vertex);
        default_vertex_pos = _default_vertex_ws_os[geom_node][vertex];
        default_vertex = default_vertex_pos[0];
        double s = default_vertex_pos[1][0];
        double t = default_vertex_pos[1][1];
        double u = default_vertex_pos[1][2];

        x_ffd = deform_vertex(s, t, u);
        rewriter.set_data3f(x_ffd);
    }
}

/*
* Deforms all vertices that are being influenced by the given control points.
*/
void FreeFormDeform::transform_vertex(GeomVertexData* data, GeomNode* geom_node, std::vector<int>& control_points) {
    GeomVertexWriter rewriter(data, "vertex");

    LPoint3f default_vertex, default_object_space; // (stu)
    LVector3f x_ffd;

    // Check who is being influenced by control points.
    std::unordered_set<int> vertices;
    for (size_t i = 0; i < control_points.size(); i++) {
        pvector<int>& influenced_arrays = _influenced_vertices[geom_node][i];
        for (int v : influenced_arrays) {
            vertices.insert(v);
        }
    }
    pvector<LPoint3f> default_vertex_pos;

    for (const int& vertex : vertices) {
        default_vertex_pos = _default_vertex_ws_os[geom_node][vertex];

        rewriter.set_row(vertex);

        default_vertex = default_vertex_pos[0];
        double s = default_vertex_pos[1][0];
        double t = default_vertex_pos[1][1];
        double u = default_vertex_pos[1][2];

        x_ffd = deform_vertex(s, t, u);
        rewriter.set_data3f(x_ffd);
    }
}

/*
* Primary deformation function. Parameters s, t, u are the
* default vertex position previously calculated in process_node.
*/
LVector3f FreeFormDeform::deform_vertex(double s, double t, double u) {
    std::vector<int>& spans = _lattice->get_edge_spans();

    double bernstein_coeff;
    int p_index = 0;

    // For nesting together all i, j, k between and including their respective spans (l, m, n)
    // Will call the berstein polynomial on the lowest and begin performing an
    // scaled multiply operation backwards.

    LVector3f vec_i = LVector3f(0);
    for (int i = 0; i <= spans[0]; i++) {
        LVector3f vec_j = LVector3f(0);
        for (int j = 0; j <= spans[1]; j++) {
            LVector3f vec_k = LVector3f(0);
            for (int k = 0; k <= spans[2]; k++) {
                bernstein_coeff = bernstein(k, 2, spans[2], u);

                vec_k += bernstein_coeff * _lattice->get_control_point_pos(p_index, _top_node);

                p_index++;
            }
            bernstein_coeff = bernstein(j, 1, spans[1], t);
            vec_j += bernstein_coeff * vec_k;
        }
        bernstein_coeff = bernstein(i, 0, spans[0], s);
        vec_i += bernstein_coeff * vec_j;
    }

    return vec_i;
}

/*
* Internally calls transform_all_influenced or transform_vertex.
* Will always call reset_vertices afterwards.
* 
* <force> argument is for when we are selecting the actual NodePath and not
* any control points. In this case, it will transform all influenced vertices
* to account for going in and out of the bounds of the lattice.
* 
* Calls lattice->update_edges.
*/
void FreeFormDeform::update_vertices(bool force) {
    std::vector<int> &control_point_indices = _lattice->get_selected_control_points();

    PT(GeomVertexData) vertex_data;
    PT(Geom) geom;

    // Iterate through geom_nodes and their child geoms:
    for (GeomNode* geom_node : _geom_nodes) {
        for (size_t i = 0; i < geom_node->get_num_geoms(); i++) {
            geom = geom_node->modify_geom(i);
            vertex_data = geom->modify_vertex_data();

            // We may be reset then come back into scope of the lattice.
            // At this point, we deform all vertices within the lattice.
            if (control_point_indices.size() == 0 && force) {
                transform_all_influenced(vertex_data, geom_node);
            }
            else {
                // Otherwise, deform only what is influenced by the selected control points.
                transform_vertex(vertex_data, geom_node, control_point_indices);
            }
            // We're going to reset the vertices that are no longer apart of the lattice.
            reset_vertices(vertex_data, geom_node, control_point_indices);
        }
    }

    // Also updates the lattice:
    for (size_t i : control_point_indices) {
        _lattice->update_edges(i);
    }
}

/*
* Primary node and vertex processing function.
* 
* Reads each Geom's vertex data. Initially, it captures the default vertex position.
* This vertex position is relative to itself, so the NodePath can always be manipulated
* before and after.
*
* - Determines if a vertex is within the bounds of the Lattice or not.
* - Converts all vertices to s,t,u space. 
* - Creates influence relationship between vertex and control point.
*/
void FreeFormDeform::process_node() {
    // Ignore if there's nothing.
    if (_geom_node_collection.get_num_paths() == 0) {
        return;
    }

    // Reset vectors/maps:
    _influenced_vertices.clear();
    _non_influenced_vertex.clear();
    _geom_nodes.clear();

    // Begin by caculating stu based on our bounding box.
    _lattice->calculate_lattice_vec();

    pvector<LVector3f> lattice_vec = _lattice->get_lattice_vecs();
    LVector3f S = lattice_vec[0];
    LVector3f T = lattice_vec[1];
    LVector3f U = lattice_vec[2];

    GeomVertexReader v_reader;
    LPoint3f vertex, vertex_minus_min;

    pvector<LPoint3f> vertex_object_space;

    CPT(GeomVertexData) vertex_data;
    PT(GeomNode) geom_node;
    CPT(Geom) geom;

    bool influenced = false;

    int row = 0;

    LVector3f x0 = _lattice->get_x0();
    LVector3f x1 = _lattice->get_x1();

    for (size_t i = 0; i < _geom_node_collection.get_num_paths(); i++) {
        geom_node = DCAST(GeomNode, _geom_node_collection.get_path(i).node());
        for (size_t j = 0; j < geom_node->get_num_geoms(); j++) {
            geom = geom_node->get_geom(j);
            vertex_data = geom->get_vertex_data();

            // Store our unmodified points:
            v_reader = GeomVertexReader(vertex_data, "vertex");
            row = 0;
            while (!v_reader.is_at_end()) {
                vertex = v_reader.get_data3f();

                // We only want the default vertices once.
                if (!captured_default_vertices) {
                    vertex_object_space.clear();

                    vertex_minus_min = vertex - _lattice->get_x0();

                    double s = (T.cross(U).dot(vertex_minus_min)) / T.cross(U).dot(S);
                    double t = (S.cross(U).dot(vertex_minus_min)) / S.cross(U).dot(T);
                    double u = (S.cross(T).dot(vertex_minus_min)) / S.cross(T).dot(U);

                    vertex_object_space.push_back(vertex);
                    vertex_object_space.push_back(LPoint3f(s, t, u));

                    _default_vertex_ws_os[geom_node].push_back(vertex_object_space);
                }

                // We do not care about vertices that aren't within our lattice.
                if (!_lattice->point_in_range(_render.get_relative_point(_np, vertex))) {
                    _non_influenced_vertex[geom_node].push_back(row);
                    row++;
                    continue;
                }

                // We're going to determine if this vertex is modified by a control point.
                for (size_t ctrl_i = 0; ctrl_i < _lattice->get_num_control_points(); ctrl_i++) {
                    influenced = is_influenced(ctrl_i, _default_vertex_ws_os[geom_node][row][1]);
                    if (influenced) {
                        _influenced_vertices[geom_node][ctrl_i].push_back(row);
                    }
                }
                row++;
            }
        }
        _geom_nodes.push_back(geom_node);
    }
    captured_default_vertices = true;
}


/*
* Outputs useful info regarding FreeFormDeform instance.
*/
std::ostream& operator<<(std::ostream& os, FreeFormDeform& obj) {
    os << "FreeFormDeform:\n";
    os << " # _geom_nodes: " << obj._geom_nodes.size() << "\n";
    os << " # _influenced_vertices[k]: " << obj._influenced_vertices.size() << "\n";
    for (GeomNode* g_n : obj._geom_nodes) {
        os << "  " << obj._influenced_vertices[g_n].size() << "\n";
    }
    os << " # _non_influenced_vertex: " << obj._non_influenced_vertex.size() << "\n";
    for (GeomNode* g_n : obj._geom_nodes) {
        os << "  " << obj._non_influenced_vertex[g_n].size() << "\n";
    }
    os << " # _default_vertex_ws_os: " << obj._default_vertex_ws_os.size() << "\n";
    for (GeomNode* g_n : obj._geom_nodes) {
        os << "  " << obj._default_vertex_ws_os[g_n].size() << "\n";
    }
    os << " # _v_n_comb_table: " << obj._v_n_comb_table.size() << "\n";
    os << " # _selected_points: " << obj._selected_points.size() << "\n";
    os << " # _geom_node_collection: " << obj._geom_node_collection.get_num_paths() << "\n";
    return os;
}
