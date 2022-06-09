import numpy as np
import pyopencl as cl
from util import create_input_memory, create_output_memory, run_ocl_kernel

if __name__ == '__main__':
    nvidia_context = cl.create_some_context()

    program_source = """
          kernel void sum(__global float *a, 
                          __global float *b,
                          __global int *c){
            int gid = get_global_id(0);
            float x = a[gid];
            float y = b[gid];
            if (x*x + y*y < 1){
                c[gid] = 1;
            }
          }
        """
    nvidia_program_source = cl.Program(nvidia_context, program_source)
    nvidia_program = nvidia_program_source.build()
    program_kernel_names = nvidia_program.get_info(cl.program_info.KERNEL_NAMES)
    print("Kernel Names: {}".format(program_kernel_names))

    # Synthetic data setup
    N = int(2 ** 26)
    G = N/2
    L = 32

    a = np.random.uniform(0, 1, N).astype(np.float32)
    b = np.random.uniform(0, 1, N).astype(np.float32)
    c = np.zeros(N).astype(np.int32)
    nvidia_queue = cl.CommandQueue(nvidia_context)

    input_tuples = create_input_memory(nvidia_context, (a, b,))
    output_tuples = create_output_memory(nvidia_context, (c,))

    run_ocl_kernel(nvidia_queue, nvidia_program.sum, (N,), input_tuples, output_tuples, local_size=(L,))

    number = float(len(c[c == 1]))
    pi = 4 * number / N
    print(pi)

