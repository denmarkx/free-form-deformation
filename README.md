# Free-Form Deformation
An work-in-progress implementation of Thomas W. Sederberg and Scott R. Parry's Free-Form Deformation of Solid Geometric Models in Panda3D.

## Implementation
Primary FFD implementation is located within `freeFormDeform.cxx` and `lattice.cxx`. 

Neighboring modules such as the `DraggableObjectManager`, `ObjectHandles`, and `LineSegs_ext` are strictly used and integrated for the purpose of visualization.

## Literature Review
**Scott Parry's Thesis:**
> [_Free-Form Deformations in a Constructive Solid Geometry Modeling System_](https://scholarsarchive.byu.edu/cgi/viewcontent.cgi?params=/context/etd/article/5254/&path_info=Scott_Parry_from_IR_problem_children.pdf)

**Sederberg and Parry's SIGGRAPH 1986 paper:**
> [_Free-Form Deformation of Solid Geometric Models_](https://people.eecs.berkeley.edu/~sequin/CS285/PAPERS/Sederberg_Parry.pdf)
