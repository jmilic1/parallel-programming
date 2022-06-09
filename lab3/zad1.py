import numpy as np
import pyopencl as cl
import time
from util import create_input_memory, create_output_memory, run_ocl_kernel


if __name__ == '__main__':
    nvidia_context = cl.create_some_context()

    program_source = """
          kernel void sum(__global int *a, 
                          __global int *b,
                          const unsigned int N){
            int gid = get_global_id(0);
            size_t gsize = get_global_size(0);
            
            int n = N/gsize;
            for (int i = n * gid; i < n * (gid+1); i++){
                int num = a[i];
                int isPrime = 1;
                if (!num){
                    isPrime = 0;
                }
                
                for (int j = 2; j < num; j++){
                    if (num % j == 0){
                        isPrime = 0;
                        break;
                    }
                }
                
                if (isPrime){
                    b[0] = b[0] + 1;
                    b[i+1] = 1;
                }
            }
          }
        """
    nvidia_program_source = cl.Program(nvidia_context, program_source)
    nvidia_program = nvidia_program_source.build()
    program_kernel_names = nvidia_program.get_info(cl.program_info.KERNEL_NAMES)
    print("Kernel Names: {}".format(program_kernel_names))

    # Synthetic data setup
    N = int(2 ** 18)
    G = N
    L = 32

    a = np.arange(1, N + 1).astype(np.int32)
    b = np.zeros(N+1).astype(np.int32)
    nvidia_queue = cl.CommandQueue(nvidia_context)

    input_tuples = create_input_memory(nvidia_context, (a,))
    output_tuples = create_output_memory(nvidia_context, (b,))
    run_ocl_kernel(nvidia_queue, nvidia_program.sum, (N,), input_tuples, output_tuples, (np.uint32(N),))

    print("non-atomic: " + str(b[0]))
    print("atomic: " + str(len(b[b == 1])))
    printOutput = ""
    for i, elem in enumerate(b):
        if elem == 1:
            printOutput += str(i) + " "
    print(printOutput)

    for g in range(1, N + 1):
        for l in range(1, 33):
            if g % l != 0:
                continue

            print(g, l)
            # Device Memory setup
            a = np.arange(1, N + 1).astype(np.int32)
            b = np.zeros(N+1).astype(np.float32)
            nvidia_queue = cl.CommandQueue(nvidia_context)

            input_tuples = create_input_memory(nvidia_context, (a,))
            output_tuples = create_output_memory(nvidia_context, (b,))

            run_ocl_kernel(nvidia_queue, nvidia_program.sum, (g,), input_tuples, output_tuples, (np.uint32(N),), (l,))
