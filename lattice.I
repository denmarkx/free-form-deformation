/*
* Constructor for Lattice. NodePath is the singular, upper level node
* of the object wanting to be deformed.
*/
inline Lattice::Lattice(NodePath np) : NodePath("FFD_Lattice"), DraggableObject() {
    _np = np;
    rebuild();
}

/*
* Deletes the Lattice entirely..including the node and control points.
*/
inline Lattice::~Lattice() {
    remove_node();
}

/*
* Returns number of contol points in the lattice.
*/
inline int Lattice::get_num_control_points() {
    return _control_points.size();
}

/*
* Returns lower boundary of given NodePath.
*/
inline LPoint3f Lattice::get_x0() const {
    return this->_x0;
}

/*
* Returns higher boundary of given NodePath.
*/
inline LPoint3f Lattice::get_x1() const{
    return this->_x1;
}

/*
* Returns vector representing s, t, u LVector3fs.
* s = Vec3(s, 0, 0);
* t = Vec3(0, 0, t);
* u = Vec3(0, u, 0);
*/
inline pvector<LVector3f> Lattice::get_lattice_vecs() const {
    return this->_lattice_vecs;
}

/*
* Sets the position of the control point given the index.
*/
inline void Lattice::set_control_point_pos(LPoint3f pos, int index) {
    NodePath c_point = _control_points[index];
    c_point.set_pos(pos);
}

/*
* Returns point of control point relative to other NodePath.
*/
inline LPoint3f &Lattice::get_control_point_pos(int i, const NodePath& other) {
    return _control_points[i].get_pos(other);
}

/*
* Returns the control point's NodePath of the given index.
*/
inline NodePath& Lattice::get_control_point(int index) {
    return _control_points[index];
}

/*
* Returns vector of size 3 representing l, m, n plane divisions.
*/
inline std::vector<int>& Lattice::get_edge_spans() {
    return _plane_spans;
}

/*
* Returns vector of size 3 representing i, j, k given the control point index.
*/
inline std::vector<int>& Lattice::get_ijk(int index) {
    return _point_ijk_map[index];
}

/*
* Returns vector of integers representing the indices of selected control points.
*/
inline std::vector<int>& Lattice::get_selected_control_points() {
    return _selected_control_points;
}

/*
* Returns boolean representing if the given point is between the NodePath's bounds.
*/
inline bool Lattice::point_in_range(LPoint3f& point) {
    return _npBoundingLarge->contains(point) ==
        (BoundingVolume::IntersectionFlags::IF_possible ^
            BoundingVolume::IntersectionFlags::IF_some ^
            BoundingVolume::IntersectionFlags::IF_all);
}
