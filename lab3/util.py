import time
import pyopencl as cl


def run_ocl_kernel(queue, kernel, global_size,
                   input_tuples, output_tuples,
                   int_constants=(), local_size=(32,)):
    # copying data onto the device
    for (array, buffer) in input_tuples:
        cl.enqueue_copy(queue, src=array, dest=buffer)

    # running program on the device
    kernel_arguments = [buffer for (_, buffer) in input_tuples]
    kernel_arguments += [buffer for (_, buffer) in output_tuples]
    kernel_arguments += [var for var in int_constants]

    start = time.time()
    kernel(queue, global_size, local_size,
           *kernel_arguments)

    # copying data off the device
    for (arr, buffer) in output_tuples:
        cl.enqueue_copy(queue, src=buffer, dest=arr)
    print("--- %s seconds ---" % (time.time() - start))

    # waiting for everything to finish
    queue.finish()


def create_input_memory(context, input_arrays):
    return [(array, cl.Buffer(context,
                              flags=cl.mem_flags.READ_ONLY,
                              size=array.nbytes))
            for array in input_arrays]


def create_output_memory(context, output_arrays):
    return [(array, cl.Buffer(context,
                              flags=cl.mem_flags.WRITE_ONLY,
                              size=array.nbytes))
            for array in output_arrays]
