import numpy as np
from boundary import boundarypsi
import math
import time
import jacobi

# 0.25011052976070103
# 0.13979117045781442
if __name__ == '__main__':
    printfreq = 1000  # output frequency
    error, bnorm = (0, 0)
    tolerance = 0.0  # tolerance for convergence. <= 0 means do not check

    # main arrays
    psi = []
    # temporary versions of main arrays
    psitmp = []

    # command line arguments
    scalefactor, numiter = (64, 1000)

    # simulation sizes
    bbase = 10
    hbase = 15
    wbase = 5
    mbase = 32
    nbase = 32

    irrotational = 1
    checkerr = 1

    m, n, b, h, w = (0, 0, 0, 0, 0)
    iter = 0
    i, j = (0, 0)

    tstart, tstop, ttot, titer = (0, 0, 0, 0)

    # do we stop because of tolerance?
    if tolerance > 0:
        checkerr = 1

    # check command line parameters and parse them

    print("Irrotational flow\n")

    # Calculate b, h & w and m & n
    b = bbase * scalefactor
    h = hbase * scalefactor
    w = wbase * scalefactor
    m = mbase * scalefactor
    n = nbase * scalefactor

    print(f"Running CFD on %d x %d grid in serial", m, n)

    # allocate arrays
    psi = np.zeros((m + 2) * (n + 2)).astype(np.float64)
    psitmp = np.zeros((m + 2) * (n + 2)).astype(np.float64)

    # set the psi boundary conditions
    psi = boundarypsi(psi, m, b, h, w)

    # compute normalisation factor for error
    bnorm = 0.0
    for i in range(m + 2):
        for j in range(m + 2):
            bnorm += psi[i * (m + 2) + j] * psi[i * (m + 2) + j]

    bnorm = math.sqrt(bnorm)

    # begin iterative Jacobi loop
    print("\nStarting main loop...\n")
    tstart = time.time()

    for iter in range(1, numiter):
        # calculate psi for next iteration
        psitmp = jacobi.jacobistep(psitmp, psi, m, n)

        # calculate current error if required
        if checkerr or iter == numiter:
            error = jacobi.deltasq(psitmp, psi, m, n)

            error = math.sqrt(error)
            error = error / bnorm
            print(error)
            error = 2

        # quit early if we have reached required tolerance
        if checkerr:
            if error < tolerance:
                print("Converged on iteration", iter)
                break

        # copy back
        for i in range(1, m + 1):
            for j in range(1, n + 1):
                psi[i * (m + 2) + j] = psitmp[i * (m + 2) + j]

        # print loop information
        if iter % printfreq == 0:
            if not checkerr:
                print("Completed iteration ", iter)
            else:
                print("fCompleted iteration %d, error = %g", iter, error)

    if iter > numiter:
        iter = numiter

    tstop = time.time()

    ttot = tstop - tstart
    titer = ttot / iter

    print("\n... finished\n")
    print("After ", iter, " iterations, the error is ", error)
