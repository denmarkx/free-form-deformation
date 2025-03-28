#include "freeFormDeform.h"

FreeFormDeform::FreeFormDeform(NodePath np, NodePath render) {
    _np = np;
    _render = render;

    _lattice = new Lattice(_np);
    _lattice->reparent_to(_render);

    process_node();
}

int FreeFormDeform::factorial(int n) {
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}

int FreeFormDeform::binomial_coeff(int n, int k) {
    return factorial(n) / (factorial(k) * factorial(n - k));
}

int FreeFormDeform::bernstein(int v, int n, int x) {
    return binomial_coeff(n, v) * pow(x, v) * pow(1 - x, n - v);
}

void FreeFormDeform::transform_vertex(GeomVertexData* data) {
    GeomVertexRewriter rewriter(data, "vertex");

    LPoint3f vertex;
    LPoint3f vertex_minus_min;

    _lattice->calculate_lattice_vec();

    pvector<LVector3f> lattice_vecs = _lattice->get_lattice_vecs();
    LVector3f S = lattice_vecs[0];
    LVector3f T = lattice_vecs[1];
    LVector3f U = lattice_vecs[2];

    LVector3f x_ffd;

    while (!rewriter.is_at_end()) {
        vertex = rewriter.get_data3f();
        vertex_minus_min = vertex - _lattice->get_x0();
        
        double s = (T.cross(U).dot(vertex_minus_min)) / T.cross(U).dot(S);
        double t = (S.cross(U).dot(vertex_minus_min)) / S.cross(U).dot(T);
        double u = (S.cross(T).dot(vertex_minus_min)) / S.cross(T).dot(U);
        x_ffd = deform_vertex(s, t, u);
        rewriter.set_data3f(x_ffd);
    }
}

LVector3f FreeFormDeform::deform_vertex(double s, double t, double u) {
    pvector<int> spans = _lattice->get_edge_spans();

    int bernstein_coeff;
    int p_index = 0;

    LVector3f vec_i = LVector3f(0);
    for (int i = 0; i <= spans[0]; i++) {
        LVector3f vec_j = LVector3f(0);
        for (int j = 0; j <= spans[1]; j++) {
            LVector3f vec_k = LVector3f(0);
            for (int k = 0; k <= spans[2]; k++) {
                bernstein_coeff = bernstein(k, spans[2], u);

                LPoint3f p_ijk = _lattice->get_control_point_pos(p_index);
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
    PT(GeomVertexData) vertex_data;
    PT(Geom) geom;

    for (GeomNode* geom_node : _geom_nodes) {
        for (size_t i = 0; i < geom_node->get_num_geoms(); i++) {
            geom = geom_node->modify_geom(i);
            vertex_data = geom->modify_vertex_data();
            transform_vertex(vertex_data);
            geom->set_vertex_data(vertex_data);
            geom_node->set_geom(i, geom);
        }
    }
}

void FreeFormDeform::process_node() {
    NodePathCollection collection = _np.find_all_matches("**/+GeomNode");
    PT(GeomNode) geom_node;

    for (size_t i = 0; i < collection.get_num_paths(); i++) {
        geom_node = DCAST(GeomNode, collection.get_path(i).node());
        _geom_nodes.push_back(geom_node);
    }
}

void FreeFormDeform::set_edge_spans(int size_x, int size_y, int size_z) {
    _lattice->set_edge_spans(size_x, size_y, size_y);
}

void FreeFormDeform::handle_click(const Event* event, void* args) {
    ClickerArgs* c_args = (ClickerArgs*)args;

    PT(MouseWatcher) mouse = c_args->mouse;
    FreeFormDeform*ffd = c_args->ffd;
    WindowFramework *window = c_args->window;

    if (!mouse->has_mouse()) {
        return;
    }

    Camera *camera = window->get_camera(0);
    ffd->_collision_ray->set_from_lens(camera, mouse->get_mouse().get_x(), mouse->get_mouse().get_y());

    NodePath render = window->get_render();
    ffd->_traverser->traverse(render);
    std::cout << ffd->_handler_queue->get_num_entries() << "\n";
}

void FreeFormDeform::setup_clicker(Camera *camera, PandaFramework &framework, WindowFramework &window) {
   _traverser = new CollisionTraverser("FFD_Traverser");

    PT(CollisionNode) collision_node = new CollisionNode("mouse_ray");
    collision_node->set_from_collide_mask(GeomNode::get_default_collide_mask());

    NodePath picker_node = window.get_camera_group().attach_new_node(collision_node);
    picker_node.show();

    _collision_ray = new CollisionRay();
    collision_node->add_solid(_collision_ray);

    _handler_queue = new CollisionHandlerQueue();
    _traverser->add_collider(picker_node, _handler_queue);

    NodePath mouse = window.get_mouse();
    PT(MouseWatcher) mouse_ptr = DCAST(MouseWatcher, mouse.node());

    ClickerArgs *c_args = new ClickerArgs{ mouse_ptr, &window, this };

    framework.define_key("mouse1", "description", handle_click, c_args);
}
