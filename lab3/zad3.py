import math
import numpy as np
import pyopencl as cl
import pyopencl.array as arr
from boundary import boundarypsi

if __name__ == '__main__':
    printfreq = 1000
    tolerance = 0.0

    # simulation sizes
    bbase = 10
    hbase = 15
    wbase = 5
    mbase = 32
    nbase = 32
    irrotational = 1
    checkerr = 0

    if tolerance > 0:
        checkerr = 1

    # scalefactor = 64, numiter = 1000 daje 0.0012553431568575791
    scalefactor = 64
    numiter = 1000

    print("Irrotational flow")

    b = bbase * scalefactor
    h = hbase * scalefactor
    w = wbase * scalefactor
    m = mbase * scalefactor
    n = nbase * scalefactor

    print("Running CFD on ", m, " x ", n, " grid in serial")

    psi = np.zeros((m + 2) * (n + 2)).astype(np.float64)
    psitmp = np.zeros((m + 2) * (n + 2)).astype(np.float64)

    psi = boundarypsi(psi, m, b, h, w)

    bnorm = 0.0
    for i in range(m + 2):
        for j in range(m + 2):
            bnorm += psi[i * (m + 2) + j] * psi[i * (m + 2) + j]

    bnorm = math.sqrt(bnorm)

    print("starting main loop...")

    for iter in range(1, numiter + 1):
        print("started iteration number ", iter)

        # initialize
        ctx = cl.create_some_context()
        queue = cl.CommandQueue(ctx)
        inputN = np.array([n]).astype(np.int32)
        inputM = np.array([m]).astype(np.int32)

        # allocate arrays
        inputPsi = arr.to_device(queue, psi)
        inputPsiTemp = arr.to_device(queue, psitmp)
        # jacobi
        prg = cl.Program(ctx, """
            __kernel void jacobi(int inputN, int inputM, __global double *psi ,__global double *psiNew)
            {
                int i = get_global_id(0) + 1;

                int n = inputN;
                int m = inputM;
                for (int j = 1; j < n + 1; ++j)
                {
                    psiNew[i * (m + 2) + j] = 0.25 * (psi[(i - 1) * (m + 2) + j] + psi[(i + 1) * (m + 2) + j] + psi[i * (m + 2) + j - 1] + psi[i * (m + 2) + j + 1]);
                }            
            }
            """).build()
        prg.jacobi(queue, (m,), None, np.uint32(n), np.uint32(m), inputPsi.data, inputPsiTemp.data)

        if checkerr == 1 or iter == numiter:
            prg = cl.Program(ctx, """
                       __kernel void deltasq(__global double *err,int inputN,int inputM, __global double *psi ,__global double *psiNew)
                       {

                           int i = get_global_id(0) + 1;

                            int n = inputN;
                            int m = inputM;
                            double dsq = 0.0;
                            double tmp;

                            for (int j = 1; j < n + 1; ++j)
                            {
                                tmp = psiNew[i * (m + 2) + j] - psi[i * (m + 2) + j];
                                dsq += tmp * tmp;    
                            }

                            err[i-1] = dsq;            
                        }
                        """).build()
            err = np.zeros(m).astype(np.float64)
            inputErr = arr.to_device(queue, err)
            prg.deltasq(queue, (m,), None, inputErr.data, inputN.data, inputM.data, inputPsi.data,
                        inputPsiTemp.data)
            error = inputErr.get().sum()
            error = math.sqrt(error)
            error = error / bnorm

        if checkerr == 1:
            if error < tolerance:
                print("Converged on iteration ", iter)
                break

        prg = cl.Program(ctx, """
            __kernel void forloop(int inputN,int inputM, __global double *psi ,__global double *psiNew)
            {

                   uint i = get_global_id(0) + 1;
                int n = inputN;
                int m = inputM;
                for (int j = 1; j < n + 1; ++j)
                {
                    psi[i * (m + 2) + j] = psiNew[i * (m + 2) + j];
                }            
            }
            """).build()
        prg.forloop(queue, (m,), None, inputN.data, inputM.data, inputPsi.data, inputPsiTemp.data)
        psi = inputPsi.get()
        psitmp = inputPsiTemp.get()

        if iter % printfreq == 0:
            if checkerr == 0:
                print("Completed iteration ", iter)
            else:
                print("Completed iteration ", iter, ", error = ", error)

    if iter < numiter:
        iter = numiter

    print("\n... finished\n")
    print("After ", iter, " iterations, the error is ", error)
    print()
