// Not necessarily a formal extension.
// This is so that we can use DraggableObject on a LineSegs.

// Alternatively, we may be able to remove all this collision picking nonsense
// and actually just turn the lines into screen space and check if we're on the line via mouse.

#include "lineSegs_ext.h"

#include "collisionCapsule.h"
#include "collisionNode.h"

/*
    Creates a collision capsule from vn -> vn+1 given a created LineSegs instance
    and the NodePath returned from attaching it elsewhere. Capsules are
    parented to the lineNP.
*/
NodePath LINESEGS_EXT::process_lines(LineSegs& line, NodePath& lineNP) {
    LPoint3f prev_vertex, vertex;

    // Remove all previous LineSeg_CollisionCapsules.
    NodePathCollection collection = lineNP.find_all_matches("**/LineSeg_CollisionCapsule");
    for (size_t i = 0; i < collection.get_num_paths(); i++) {
        collection.get_path(i).remove_node();
    }

    for (size_t i = 0; i < line.get_num_vertices(); i++) {
        vertex = line.get_vertex(i);

        if ((i + 1) % 2 == 0) {
            prev_vertex = line.get_vertex(i - 1);
            PT(CollisionCapsule) capsule = new CollisionCapsule(vertex, prev_vertex, 0.1);

            PT(CollisionNode) coll_node = new CollisionNode("LineSeg_CollisionCapsule");
            coll_node->add_solid(capsule);

            NodePath coll_np = lineNP.attach_new_node(coll_node);
            coll_np.node()->set_into_collide_mask(CollideMask::bit(20));
        }
    }
    return NodePath();
}

/*
    Modifies the collision capsule to whatever vn -> vn+1 for the given LineSegs.
*/
void LINESEGS_EXT::update_lines(LineSegs& line, NodePath& lineNP) {
    LPoint3f prev_vertex, vertex;
    NodePathCollection collection = lineNP.find_all_matches("**/+CollisionNode");
    NodePath np;
    PT(CollisionCapsule) capsule;
    PT(CollisionNode) coll_node;

    for (size_t i = 0; i < line.get_num_vertices(); i++) {
        vertex = line.get_vertex(i);

        if ((i + 1) % 2 == 0) {
            prev_vertex = line.get_vertex(i - 1);
            np = collection[floor(i / 2)];

            // Cast back down to a node:
            coll_node = DCAST(CollisionNode, np.node());

            // Then get our capsule:
            capsule = DCAST(CollisionCapsule, coll_node->modify_solid(0));
            capsule->set_point_a(prev_vertex);
            capsule->set_point_b(vertex);
        }
    }
}
