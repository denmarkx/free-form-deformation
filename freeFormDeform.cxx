#include "freeFormDeform.h"
#include <unordered_set>

static void handle_drag(const Event* e, void* args);

FreeFormDeform::FreeFormDeform(NodePath np, NodePath render) {
    _np = np;
    _render = render;

    _lattice = new Lattice(_np);
    _lattice->reparent_to(_render);
    _lattice->hook_drag_event("FFD_DRAG_EVENT", handle_drag, this);
    process_node();
}

static void handle_drag(const Event* e, void* args) {
    FreeFormDeform* ffd = (FreeFormDeform*)args;
    ffd->process_node();
    ffd->update_vertices();
}

FreeFormDeform::~FreeFormDeform() {
    // Delete the Lattice:
    delete _lattice;
}

double FreeFormDeform::factorial(double n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}

int FreeFormDeform::binomial_coeff(int n, int k) {
    return factorial(n) / (factorial(k) * factorial(n - k));
}

double FreeFormDeform::bernstein(double v, double n, double x) {
    return binomial_coeff(n, v) * pow(x, v) * pow(1 - x, n - v);
}

/*
Returns the actual index of the control point given i, j, k.
*/
int FreeFormDeform::get_point_index(int i, int j, int k) {
    pvector<int> spans = _lattice->get_edge_spans();
    return i * (spans[2] + 1) * (spans[2] + 1) + j * (spans[1] + 1) + k;
}

/*
Returns i, j, k given the index.
*/
std::vector<int> FreeFormDeform::get_ijk(int index) {
    pvector<int> spans = _lattice->get_edge_spans();
    int l = spans[0];
    int m = spans[1];
    int n = spans[2];

    int i = (index / ((n + 1) * (m + 1))) % (l + 1);
    int j = (index / (n + 1)) % (m + 1);
    int k = (index % (n + 1));

    std::vector<int> ijk = { i, j, k };
    return ijk;
}

/*
Returns true/false if the given control point influences the given vertex (stu).
*/
bool FreeFormDeform::is_influenced(int index, double s, double t, double u) {
    pvector<int> spans = _lattice->get_edge_spans();
    NodePath &point_np = _lattice->get_control_point(index);

    std::vector<int> ijk = get_ijk(index);
    return bernstein(ijk[0], spans[0], s) * bernstein(ijk[1], spans[1], t) * bernstein(ijk[2], spans[2], u);
}

/*

*/
void FreeFormDeform::reset_vertices(GeomVertexData* data, GeomNode* geom_node, std::vector<int>& indices) {
    if (_non_influenced_vertex.find(geom_node) == _non_influenced_vertex.end()) {
        return;
    }

    GeomVertexWriter rewriter(data, "vertex");
    LPoint3f default_vertex_pos;
    
    // Now, we want to iterate through vertices that are NOT affected at all.
    int vertex_count = 0;
 
    for (size_t i = 0; i < _non_influenced_vertex[geom_node].size(); i++) {
        int t = _non_influenced_vertex[geom_node][i];
        rewriter.set_row(t);
        default_vertex_pos = _default_vertex_ws_os[geom_node][t][0];
        rewriter.set_data3f(default_vertex_pos);
    }
}

/*
*/
void FreeFormDeform::transform_all_influenced(GeomVertexData* data, GeomNode* geom_node) {
    GeomVertexWriter rewriter(data, "vertex");
    LPoint3f default_vertex, default_object_space; // (stu)
    LVector3f x_ffd;

    pmap<int, pvector<int>> influence_map = _influenced_vertices[geom_node]; // key is ctrl point.
    pvector<int> vertices;
    pvector<LPoint3f> default_vertex_pos;

    std::unordered_set<int> _vertices2;

    int vertex = 0;
    for (int i = 0; i < influence_map.size(); i++) {
        vertices = influence_map[i];
        for (int j = 0; j < vertices.size(); j++) {
            vertex = vertices[j];
            _vertices2.insert(vertex);
        }
    }

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

void FreeFormDeform::transform_vertex(GeomVertexData* data, GeomNode* geom_node, int index) {
    GeomVertexWriter rewriter(data, "vertex");

    LPoint3f default_vertex, default_object_space; // (stu)
    LVector3f x_ffd;

    pvector<int> influenced_arrays = _influenced_vertices[geom_node][index];
    pvector<LPoint3f> default_vertex_pos;

    int vertex = 0;
    for (int i = 0; i < influenced_arrays.size(); i++) {
        vertex = influenced_arrays[i];
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

LVector3f FreeFormDeform::deform_vertex(double s, double t, double u) {
    pvector<int> spans = _lattice->get_edge_spans();

    double bernstein_coeff;
    int p_index = 0;

    LVector3f vec_i = LVector3f(0);
    for (int i = 0; i <= spans[0]; i++) {
        LVector3f vec_j = LVector3f(0);
        for (int j = 0; j <= spans[1]; j++) {
            LVector3f vec_k = LVector3f(0);
            for (int k = 0; k <= spans[2]; k++) {
                bernstein_coeff = bernstein(k, spans[2], u);

                LPoint3f p_ijk = _lattice->get_control_point_pos(p_index, _lattice->_edgesNp);
                vec_k += bernstein_coeff * p_ijk;

                p_index++;
            }
            bernstein_coeff = bernstein(j, spans[1], t);
            vec_j += bernstein_coeff * vec_k;
        }
        bernstein_coeff = bernstein(i, spans[0], s);
        vec_i += bernstein_coeff * vec_j;
    }

    return vec_i;
}

void FreeFormDeform::update_vertices() {
    std::vector<int> control_point_indices;
    
    for (NodePath& np : _lattice->get_selected()) {
        if (np.get_name() == "lattice_edges") {
            break;
        }
        control_point_indices.push_back(atoi(np.get_net_tag("control_point").c_str()));
    }

    // Moving entire lattice:
    if (control_point_indices.size() == 0) {
        _np.set_pos(_lattice->_edge_pos);
        return;
    }

    PT(GeomVertexData) vertex_data;
    PT(Geom) geom;

    // TODO: outside of scope of draggable-object-mgr branch,
    // but we apparently don't need to run set_geom all the time.
    // there's some optimizations to be made here and in transform_vertex.
    for (GeomNode* geom_node : _geom_nodes) {
        for (size_t i = 0; i < geom_node->get_num_geoms(); i++) {
            geom = geom_node->modify_geom(i);
            vertex_data = geom->modify_vertex_data();

            // We may be reset then come back into scope of the lattice.
            // At this point, we deform all vertices within the lattice.
            if (control_point_indices.size() == 0) {
                // TODO: remove
                // transform_all_influenced(vertex_data, geom_node);
            }
            else {
                // Otherwise, deform only what is influenced by the selected control points.
                for (size_t j = 0; j < control_point_indices.size(); j++) {
                    transform_vertex(vertex_data, geom_node, control_point_indices[j]);
                }
            }
            // We're going to reset the vertices that are no longer apart of the lattice.
            reset_vertices(vertex_data, geom_node, control_point_indices);
            geom->set_vertex_data(vertex_data);
            geom_node->set_geom(i, geom);
        }
    }

    // Also updates the lattice:
    for (size_t i : control_point_indices) {
        _lattice->update_edges(i);
    }
}

/*
*/
void FreeFormDeform::process_node() {
    NodePathCollection collection = _np.find_all_matches("**/+GeomNode");

    // Ignore if there's nothing.
    if (collection.get_num_paths() == 0) {
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

    for (size_t i = 0; i < collection.get_num_paths(); i++) {
        geom_node = DCAST(GeomNode, collection.get_path(i).node());
        for (size_t j = 0; j < geom_node->get_num_geoms(); j++) {
            geom = geom_node->get_geom(j);
            vertex_data = geom->get_vertex_data();

            // Store our unmodified points:
            v_reader = GeomVertexReader(vertex_data, "vertex");
            row = 0;
            while (!v_reader.is_at_end()) {
                vertex_object_space.clear();
                vertex = v_reader.get_data3f();

                vertex_minus_min = vertex - _lattice->get_x0();

                double s = (T.cross(U).dot(vertex_minus_min)) / T.cross(U).dot(S);
                double t = (S.cross(U).dot(vertex_minus_min)) / S.cross(U).dot(T);
                double u = (S.cross(T).dot(vertex_minus_min)) / S.cross(T).dot(U);

                vertex_object_space.push_back(vertex);
                vertex_object_space.push_back(LPoint3f(s, t, u));

                // We only want the default vertices once.
                if (!captured_default_vertices) {
                    _default_vertex_ws_os[geom_node].push_back(vertex_object_space);
                }
                // We do not care about vertices that aren't within our lattice.
                if (!_lattice->point_in_range(vertex)) {
                    _non_influenced_vertex[geom_node].push_back(row);
                    row++;
                    continue;
                }

                // We're going to determine if this vertex is modified by a control point.
                for (size_t ctrl_i = 0; ctrl_i < _lattice->get_num_control_points(); ctrl_i++) {
                    influenced = is_influenced(ctrl_i, s, t, u);
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

void FreeFormDeform::set_edge_spans(int size_x, int size_y, int size_z) {
    _lattice->set_edge_spans(size_x, size_y, size_y);
}

LPoint3f FreeFormDeform::point_at_axis(double axis_value, LPoint3f point, LVector3f vector, int axis) {
    return point + vector * ((axis_value - point[axis]) / vector[axis]);
}

