#include "draggableObject.h"

/*
Initializer for DraggableObject. Parent is the top level NodePath of the object(s)
wanting to be apart of the draggable object. traverse_num is how far down
we traverse the children.

For example, if traverse_num==0: we only will pay attention to parent. However, if parent is just
a holding node, i.e. empty, this is useless.

If traverse_num == 2, we will watch all our children (1) and all their children (2).
*/
DraggableObject::DraggableObject(NodePath& parent, int traverse_num) {
    watch_node_path(parent, traverse_num);
}

/*
Initializer for DraggableObject. Watches for all nodes with the given tag name set via setTag.
*/
DraggableObject::DraggableObject(std::string tag) {
    _tag = tag;
}

/*
Blank initializer for DraggableObject. This is usually for when we want to set
the nodepath later on.
*/
DraggableObject::DraggableObject() {}

/*

*/
void DraggableObject::watch_node_path(NodePath& parent, int traverse_num) {
    _parent = parent;
    _traverse_num = traverse_num;
    traverse();
}

/*
Only called when DraggableObject is given a parent and a traverse num.
Keeps an internal list of  the children to make matching easier for the DOM.
*/
void DraggableObject::traverse() {
     // If traverse_num is 0, we just want the parent.
    if (_traverse_num == 0) {
        _nodes.push_back(_parent);
        return;
    }

    // Otherwise, we need to iterate through everyone traverse_num times.
    traverse_children(_parent.get_children(), 1);
}

/*
Recursively traverses children and pushes to _nodes vector.
count indicates the current traversal cycle of a single node.
Stops for each child when count == the given traverse num from init. 
*/
void DraggableObject::traverse_children(NodePathCollection& children, int count) {
    // Iterate through children.
    for (size_t i = 0; i < children.get_num_paths(); i++) {
        // Push child to vector.
        _nodes.push_back(children.get_path(i));

        // Determine if we have reached our limit.
        if (count == _traverse_num) {
            continue;
        }

        // Otherwise, keep going.
        traverse_children(children.get_path(i).get_children(), count + 1);
    }
}

/*
Returns boolean representing if we were set via DraggableObject(string tag) or not.
i.e. if tag is "".
*/
bool DraggableObject::is_watching_tag() const {
    return _tag != "";
}

/*
Returns tag set by DraggableObject(string tag).
*/
std::string DraggableObject::get_tag() const {
    return _tag;
}
