#ifndef DRAGGABLE_OBJECT_H
#define DRAGGABLE_OBJECT_H

#include "nodePath.h"
#include "eventHandler.h"

class DraggableObject {
public:
    DraggableObject(NodePath& parent, int traverse_num);
    DraggableObject(NodePath& parent, std::string tag);
    DraggableObject();
    ~DraggableObject();

    void hook_drag_event(std::string event_name, EventHandler::EventCallbackFunction function, void* args);
    void watch_node_path(NodePath& parent, int traverse_num);
    bool is_watching_tag() const;
    bool has_node(NodePath& np);
    std::string get_event_name() const;
    std::string get_tag() const;
    pvector<NodePath> get_selected();
    bool has_selected() const;

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
    std::string _event_name;

    pvector<NodePath> _nodes;
    pvector<NodePath> _selected;
};

#endif