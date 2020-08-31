# expected variable names:
#     - points -- for input matrix (list of list of integers)
#     - m      -- for mod

# credit: https://ask.sagemath.org/question/33890/how-to-find-kernel-of-a-matrix-in-mathbbzn/?answer=33950#post-id-33950

import traceback
from sage.all import *

# NOTE Following function can be implemented in C++ too
def rearrange(soln, m):
    # format of input soln (for 2 variables):
    #   [ 1 x y ] * soln = 0
    #   soln[0] => constant
    #   soln[1] => coefficient of x
    #   soln[2] => coefficient of y
    #
    # re-arrange to move constant term to last and type cast to python integer
    # output:
    #   soln[0] = 1 (or even) => coefficient of x
    #   soln[1] => coefficient of y
    #   soln[2] => constant
    soln = [int(t) for t in soln[1:]] + [int(soln[0])]
    return soln
    ## move the first coefficent to other side of '='
    #cx = (m-soln[0])%m
    #try:
    #    icx = int(inverse_mod(cx, m))
    #    assert icx != None
    #    # multiplicative inverse exists
    #    # make the coefficient of first term 1 by multiplying with inverse
    #    ns = [cx] + list(soln[1:])
    #    return [(e*icx)%m for e in ns]
    #except:
    #    return [cx] + soln[1:]

def clip_last_zeros(soln, num_vars, diff):
  assert all(elem == 0 for elem in soln[num_vars:]), "The coeff for extra variables added must be zero"
  return soln[ : num_vars]

def transpose(points):
    return [[row[i] for row in points] for i in range(len(points[0]))]

def bv_solve(points, m):
    num_pts = len(points)
    Zn = FreeModule(ZZ, num_pts)
    M  = Zn/(m*Zn)
    pointsT = transpose(points)
    num_vars = len(pointsT)

    # add zero columns if required to make square matrix
    zero_list = [0] * num_pts
    diff = num_pts - num_vars
    for i in range(diff):
      pointsT.append(zero_list)

    A =  matrix(pointsT)
    phi = M.hom([M(a) for a in A])
    # print "Nullspace(kernel):"
    # print [M(b) for b in phi.kernel()]
    # print "Basis:"
    basis = [M(b) for b in phi.kernel().gens()]

    # ignore the solution if it has non-zero coeff for only added variables
    basis = filter(lambda s: not all(elem == 0 for elem in s[:num_vars]), basis)

    ret = []
    for s in basis:
      # drop the last (num_pts -num_vars) zeros added
      s_crop = clip_last_zeros(s, num_vars, diff)
      # move constant term to the last
      soln = rearrange(s_crop, m)
      ret.append(soln)
    return ret

def bv_solve_wrapper(points,m):
    rows = len(points)
    cols = len(points[0])
    assert rows >= cols
    try:
        ret = bv_solve(points,m)
    except:
        print traceback.format_exc()
    if ret == []:
        print "[]"
    for s in ret:
        print "[ ",
        for e in s:
            print e,
        print "] ",
