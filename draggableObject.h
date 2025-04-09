#ifndef DRAGGABLE_OBJECT_H
#define DRAGGABLE_OBJECT_H

#include "nodePath.h"

class DraggableObject {
public:
    DraggableObject(NodePath& parent, int traverse_num);
    DraggableObject(NodePath& parent, std::string tag);
    DraggableObject();

    void watch_node_path(NodePath& parent, int traverse_num);
    bool is_watching_tag() const;
    std::string get_tag() const;

    virtual void select(NodePath& np);
    virtual void deselect(NodePath& np);
    void deselect();

private:
    void traverse();
    void traverse_children(NodePathCollection& children, int count);

private:
    NodePath _parent;
    int _traverse_num;
    std::string _tag;

    pvector<NodePath> _nodes;
};

#endif