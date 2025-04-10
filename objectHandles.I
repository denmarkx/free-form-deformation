/*
Sets length of object handles. Rebuilds automatically.
*/
inline void ObjectHandles::set_length(double length) {
    _length = length;
    rebuild();
}

/*
Returns double value describing current length of handles.
*/
inline double ObjectHandles::get_length() const {
    return _length;
}

/*
Sets thickness of the line segments. Rebuilds automatically.
*/
inline void ObjectHandles::set_thickness(double thickness) {
    _thickness = thickness;
    rebuild();
}

/*
Returns double value describing current thickness of handles.
*/
inline double ObjectHandles::get_thickness() const {
    return _thickness;
}

/*
Sets active flag on handles, automatically hides/shows.
*/
inline void ObjectHandles::set_active(bool active) {
    _active = active;

    // NodePath.hide/show:
    if (!active) {
        hide();
        clear_node_paths();
    } else {
        show();
    }
}

/*
Returns boolean representing if the object handles are shown or not.
*/
inline bool ObjectHandles::is_active() const {
    return _active;
}
