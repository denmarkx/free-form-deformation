#include "freeFormDeform.h"

FreeFormDeform::FreeFormDeform(NodePath np) {
    _np = np;
    process_node();
}

void FreeFormDeform::process_node() {
    NodePathCollection collection = _np.find_all_matches("**/+GeomNode");

    _lattice = new Lattice(_np);

    PT(GeomVertexData) vertex_data;
    PT(GeomNode) geom_node;
    PT(Geom) geom;

    for (size_t i = 0; i < collection.get_num_paths(); i++) {
        geom_node = DCAST(GeomNode, collection.get_path(i).node());
        for (size_t j = 0; j < geom_node->get_num_geoms(); j++) {
            geom = geom_node->modify_geom(j);
            vertex_data = geom->modify_vertex_data();
            _vertex_data.push_back(vertex_data);
            _geoms.push_back(geom);
        }
    }
}

void FreeFormDeform::set_edge_spans(int size_x, int size_y, int size_z) {
    _lattice->set_edge_spans(size_x, size_y, size_y);
}


void FreeFormDeform::set_edge_spans_DEBUG() {
    _lattice->set_edge_spans(2, 3, 4);
}
