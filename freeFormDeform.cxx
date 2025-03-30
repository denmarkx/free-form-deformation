#include "freeFormDeform.h"

FreeFormDeform::FreeFormDeform(NodePath np, NodePath render) {
    _np = np;
    _render = render;

    _lattice = new Lattice(_np);
    _lattice->reparent_to(_render);

    process_node();
}

FreeFormDeform::~FreeFormDeform() {

    if (_task_mgr->find_task("DragWatcherTask") != nullptr) {
        _task_mgr->remove(_clicker_task);
    }

    // Ignore mouse1:
    EventHandler* event_handler = EventHandler::get_global_event_handler();
    event_handler->remove_hooks_with((void*)_c_args);

    // Delete the Lattice:
    delete _lattice;

    // Collision / Clicking Data:
    _picker_node.remove_node();
    delete _c_args;
    delete _traverser;
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

double FreeFormDeform::bernstein(double v, double n, double x) {
    return binomial_coeff(n, v) * pow(x, v) * pow(1 - x, n - v);
}

void FreeFormDeform::transform_vertex(GeomVertexData* data) {
    GeomVertexRewriter rewriter(data, "vertex");

    LPoint3f default_vertex, default_object_space; // (stu)

    LVector3f x_ffd;

    int i = 0;
    while (!rewriter.is_at_end()) {
        default_vertex = _default_vertex_ws_os[i][0];
        double s = _default_vertex_ws_os[i][1][0];
        double t = _default_vertex_ws_os[i][1][1];
        double u = _default_vertex_ws_os[i][1][2];
        x_ffd = deform_vertex(s, t, u);

        // Ignore rewriting if we moved a neglible amount.
        if (!default_vertex.almost_equal(x_ffd, 0.01)) {
            rewriter.set_data3f(x_ffd);
        }
        else {
            rewriter.set_data3f(default_vertex);
        }

        i++;
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
    _lattice->calculate_lattice_vec();

    NodePathCollection collection = _np.find_all_matches("**/+GeomNode");

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

    for (size_t i = 0; i < collection.get_num_paths(); i++) {
        geom_node = DCAST(GeomNode, collection.get_path(i).node());
        for (size_t j = 0; j < geom_node->get_num_geoms(); j++) {
            geom = geom_node->get_geom(j);
            vertex_data = geom->get_vertex_data();

            // Store our unmodified points:
            v_reader = GeomVertexReader(vertex_data, "vertex");

            while (!v_reader.is_at_end()) {
                vertex_object_space.clear();
                vertex = v_reader.get_data3f();

                vertex_minus_min = vertex - _lattice->get_x0();

                double s = (T.cross(U).dot(vertex_minus_min)) / T.cross(U).dot(S);
                double t = (S.cross(U).dot(vertex_minus_min)) / S.cross(U).dot(T);
                double u = (S.cross(T).dot(vertex_minus_min)) / S.cross(T).dot(U);

                vertex_object_space.push_back(vertex);
                vertex_object_space.push_back(LPoint3f(s, t, u));
                _default_vertex_ws_os.push_back(vertex_object_space);
            }
        }
        _geom_nodes.push_back(geom_node);
    }
}

void FreeFormDeform::set_edge_spans(int size_x, int size_y, int size_z) {
    _lattice->set_edge_spans(size_x, size_y, size_y);
}

LPoint3f FreeFormDeform::point_at_axis(double axis_value, LPoint3f point, LVector3f vector, int axis) {
    return point + vector * ((axis_value - point[axis]) / vector[axis]);
}

AsyncTask::DoneStatus FreeFormDeform::drag_task(GenericAsyncTask* task, void* args) {
    ClickerArgs* c_args = (ClickerArgs*)args;

    PT(MouseWatcher) mouse = c_args->mouse;
    FreeFormDeform* ffd = c_args->ffd;
    WindowFramework* window = c_args->window;

    // TODO: checck if dragging?>

    // Ignore if mouse is out of bounds:
    if (!mouse->has_mouse()) {
        return AsyncTask::DS_again;
    }
    
    // Ignore if there's nothing selected.
    if (ffd->_selected_points.size() == 0) {
        return AsyncTask::DS_again;
    }

    NodePath render = window->get_render();
    NodePath camera_np = window->get_camera_group();

    LPoint3f near_point = render.get_relative_point(camera_np, ffd->_collision_ray->get_origin());
    LVector3f near_vector = render.get_relative_vector(camera_np, ffd->_collision_ray->get_direction());

    // XXX: temporarily forced to Y assuming looking at XZ (default)
    int index = ffd->_selected_points[0];
    LPoint3f point = ffd->_lattice->get_control_point_pos(index);

    ffd->_lattice->set_control_point_pos(ffd->point_at_axis(.5, near_point, near_vector, 1), index);

    ffd->update_vertices();
    return AsyncTask::DS_cont;
}

void FreeFormDeform::handle_click(const Event* event, void* args) {
    ClickerArgs* c_args = (ClickerArgs*)args;

    PT(MouseWatcher) mouse = c_args->mouse;
    FreeFormDeform*ffd = c_args->ffd;
    WindowFramework *window = c_args->window;

    if (!mouse->has_mouse()) {
        return;
    }

    PT(Camera) camera = window->get_camera(0);
    ffd->_collision_ray->set_from_lens(camera, mouse->get_mouse().get_x(), mouse->get_mouse().get_y());

    NodePath render = window->get_render();
    ffd->_traverser->traverse(render);
    ffd->_handler_queue->sort_entries();

    if (ffd->_handler_queue->get_num_entries() == 0) {
        return;
    }

    NodePath into_node = ffd->_handler_queue->get_entry(0)->get_into_node_path();

    if (!into_node.has_net_tag("control_point")) {
        return;
    }

    int point_index = atoi(into_node.get_net_tag("control_point").c_str());
    
    pvector<int>::iterator it;
    it = std::find(ffd->_selected_points.begin(), ffd->_selected_points.end(), point_index);

    size_t i;
    if (it != ffd->_selected_points.end()) {
        i = std::distance(ffd->_selected_points.begin(), it);

        // Deselect:
        ffd->_selected_points.erase(ffd->_selected_points.begin() + i);
        into_node.clear_color();
        return;
    }

    // Select:
    ffd->_selected_points.push_back(point_index);
    into_node.set_color(0, 1, 0, 1);
}

void FreeFormDeform::setup_clicker(WindowFramework &window) {
   _traverser = new CollisionTraverser("FFD_Traverser");

    PT(CollisionNode) collision_node = new CollisionNode("mouse_ray");
    collision_node->set_from_collide_mask(GeomNode::get_default_collide_mask());

    _picker_node = window.get_camera_group().attach_new_node(collision_node);

    _collision_ray = new CollisionRay();
    collision_node->add_solid(_collision_ray);

    _handler_queue = new CollisionHandlerQueue();
    _traverser->add_collider(_picker_node, _handler_queue);

    NodePath mouse = window.get_mouse();
    PT(MouseWatcher) mouse_ptr = DCAST(MouseWatcher, mouse.node());

    // Click mouse1 Event:
    _c_args = new ClickerArgs{ mouse_ptr, &window, this };

    EventHandler *event_handler = EventHandler::get_global_event_handler();
    event_handler->add_hook("mouse1", handle_click, _c_args);

    // Dragging Task:
    _clicker_task = new GenericAsyncTask("DragWatcherTask", &drag_task, _c_args);
    _task_mgr->add(_clicker_task);
}
