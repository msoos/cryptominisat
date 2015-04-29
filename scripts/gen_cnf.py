def gen_cnf(n):
    A = random_matrix(GF(2),n,n+1)
    B = BooleanPolynomialRing(n,'x')
    v = Matrix(B, n+1, 1, B.gens() + (1,))
    l = (A*v).list()
    a2 = ANFSatSolver(B)
    s = a2.cnf(l)
    open("matrix%02d.cnf"%n,"w").write(s)
