/*
* Deconstructor for FreeFormDeform. Removes Lattice.
*/
inline FreeFormDeform::~FreeFormDeform() {
    // Delete the Lattice:
    delete _lattice;
}

/*
* Sets the edge spans of the lattice. Lattice is automatically rebuilt.
* x, y, z is equivalent to l, n, m in Sederberg/Parry's paper.
*/
inline void FreeFormDeform::set_edge_spans(int x, int y, int z) {
    _lattice->set_edge_spans(x, y, z);
}

/*
* Simple factorial implementation of the binomial coefficients.
* https://en.wikipedia.org/wiki/Binomial_coefficient
*/
inline int FreeFormDeform::binomial_coeff(int n, int k) {
    return factorial(n) / (factorial(k) * factorial(n - k));
}

/*
* Simple implementation of the Berstein basis polynomial.
* https://en.wikipedia.org/wiki/Bernstein_polynomial#Bernstein_basis_polynomials
*/
inline double FreeFormDeform::bernstein(double v, double n, double x) {
    return binomial_coeff(n, v) * pow(x, v) * pow(1 - x, n - v);
}
