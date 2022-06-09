import math
import numpy as np
import pyopencl as cl
from boundary import boundarypsi
from util import run_ocl_kernel, create_input_memory, create_output_memory

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
    checkerr = 1

    if tolerance > 0:
        checkerr = 1

    scalefactor = 64
    numiter = 1000

    print("Irrotational flow")

    b = bbase * scalefactor
    h = hbase * scalefactor
    w = wbase * scalefactor
    m = mbase * scalefactor
    n = nbase * scalefactor

    print("Running CFD on ", m, " x ", n, " grid in serial")

    psi_in = np.zeros((m + 2) * (n + 2)).astype(np.float64)
    psi_out = np.zeros((m + 2) * (n + 2)).astype(np.float64)

    psi_in = boundarypsi(psi_in, m, b, h, w)

    bnorm = 0.0
    for i in range(m + 2):
        for j in range(m + 2):
            bnorm += psi_in[i * (m + 2) + j] * psi_in[i * (m + 2) + j]

    bnorm = math.sqrt(bnorm)

    print("starting main loop...")

    for iter in range(1, numiter + 1):
        print("started iteration number ", iter)

        # initialize
        ctx = cl.create_some_context()
        queue = cl.CommandQueue(ctx)

        psi_in_cl = create_input_memory(ctx, (psi_in,))
        psi_out_cl = create_output_memory(ctx, (psi_out,))
        # jacobi
        prg = cl.Program(ctx, """
            __kernel void jacobi(__global double *psi, __global double *psiNew, int inputN, int inputM)
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
        run_ocl_kernel(queue, prg.jacobi, (m,), psi_in_cl, psi_out_cl, (np.uint32(n), np.uint32(m),), None)

        if checkerr == 1 or iter == numiter:
            prg = cl.Program(ctx, """
                       __kernel void deltasq(__global double *psi ,__global double *psiNew, __global double *err, int inputN, int inputM)
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
            delta_in_cl = create_input_memory(ctx, (psi_in, psi_out,))
            err = np.zeros(m).astype(np.float64)
            delta_out_cl = create_output_memory(ctx, (err,))

            run_ocl_kernel(queue, prg.deltasq, (m, ), delta_in_cl, delta_out_cl, (np.uint32(n), np.uint32(m),), None)
            error = err.sum()
            error = math.sqrt(error)
            error = error / bnorm

            if checkerr == 1:
                if error < tolerance:
                    print("Converged on iteration ", iter)
                    break

            prg = cl.Program(ctx, """
            __kernel void forloop(__global double *psi ,__global double *psiNew, int inputN,int inputM)
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
            for_in_cl = create_input_memory(ctx, (psi_in,))
            for_out_cl = create_output_memory(ctx, (psi_out,))
            run_ocl_kernel(queue, prg.forloop, (m,), for_in_cl, for_out_cl, (np.uint32(n), np.uint32(m),), None)

            if iter % printfreq == 0:
                if checkerr == 0:
                    print("Completed iteration ", iter)
                else:
                    print("Completed iteration ", iter, ", error = ", error)

            print("\n... finished\n")
            print("After ", iter, " iterations, the error is ", error)
            print()
