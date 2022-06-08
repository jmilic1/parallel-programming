#include <mpi.h>
#include <stdio.h>
#include "../helloWorld.c"
#include "../sendRecieve.c"
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

struct Fork {
    int heldBy;
    bool dirty;
};

struct Philosopher {
    int id;
    int left_neighbor_id;
    int right_neighbor_id;
    bool *left_requested;
    bool *right_requested;
    struct Fork *left_fork;
    struct Fork *right_fork;
};

void printNTabs(int n){
    for (int i = 0; i < n; i++){
        printf("\t");
    }
}

int getLeftIndex(int world_rank, int world_size) {
    if (world_rank == 0) {
        return world_size - 1;
    }
    return world_rank - 1;
}

int getRightIndex(int world_rank, int world_size) {
    return (world_rank + 1) % world_size;
}

void probe(int id, int neighbor_id, struct Fork *fork, bool *request_storage){
    // reading messages from left neighbour

    int flag = 0;
    MPI_Iprobe(neighbor_id, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);

    bool debugNeighborIsLeft = (id == 0 && neighbor_id != 1) || (neighbor_id == id - 1);
    if (flag == 1) {
        int ignore;
        MPI_Recv(&ignore, 1, MPI_INT, neighbor_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (fork->heldBy != id) {
            fork->heldBy = id;

        } else if (fork->dirty) {
            fork->dirty = false;
            fork->heldBy = -1;
            MPI_Send(&id, 1, MPI_INT, neighbor_id, 0, MPI_COMM_WORLD);
        } else {
            *request_storage = true;
        }
    }
}

void my_listen(struct Philosopher *philosopher){
    int id = philosopher->id;
    struct Fork *left_fork = philosopher->left_fork;
    struct Fork *right_fork = philosopher->right_fork;
    int left_neighbor = philosopher->left_neighbor_id;
    int right_neighbor = philosopher->right_neighbor_id;
    bool *left_requested = philosopher->left_requested;
    bool *right_requested = philosopher->right_requested;

    if (*left_requested && left_fork->heldBy != id) {
        printf("Error! I am %d and I got a request from left, but I don't have fork\n", id);
        printf("%d: left_requested = %d, left_fork->heldBy: %d", id, *left_requested, left_fork->heldBy);
        printf("Terminate me!");
        sleep(2);
    }

    if (*left_requested && left_fork->dirty) {
        left_fork->heldBy = -1;
        left_fork->dirty = false;
        *left_requested = false;
        MPI_Send(&id, 1, MPI_INT, philosopher->left_neighbor_id, 0, MPI_COMM_WORLD);
    }

    if (*right_requested && right_fork->heldBy != id) {
        printf("Error! I am %d and I got a request from right, but I don't have fork\n", id);
        printf("%d: right_requested = %d, right_fork->heldBy: %d", id, *right_requested, left_fork->heldBy);
        printf("Terminate me!");
        sleep(2);
    }
    if (*right_requested && right_fork->dirty) {
        right_fork->heldBy = -1;
        right_fork->dirty = false;
        *right_requested = false;
        MPI_Send(&id, 1, MPI_INT, philosopher->right_neighbor_id, 0, MPI_COMM_WORLD);
    }

    // probe left neighbor
    probe(id, left_neighbor, left_fork, left_requested);

    // probe right neighbor
    probe(id, right_neighbor, right_fork, right_requested);
}

void think(struct Philosopher *philosopher) {
    int count = rand() % 5 + 1;
    printNTabs(philosopher->id);
    printf("%d: thinking\n", philosopher->id);

    while (count > 0) {
        my_listen(philosopher);
        sleep(1);
        count--;
    }
}

void getForks(struct Philosopher *philosopher){
    int id = philosopher->id;
    struct Fork *left_fork = philosopher->left_fork;
    struct Fork *right_fork = philosopher->right_fork;
    int left_neighbor = philosopher->left_neighbor_id;
    int right_neighbor = philosopher->right_neighbor_id;
    bool *left_requested = philosopher->left_requested;
    bool *right_requested = philosopher->right_requested;

    printNTabs(id);
    while (left_fork->heldBy != id || right_fork->heldBy != id){
        if (left_fork->heldBy != id){
            printNTabs(id);
            printf("%d: requesting left fork\n", id);

            MPI_Send(&id, 1, MPI_INT, left_neighbor, 0, MPI_COMM_WORLD);

            while (left_fork->heldBy != id){
                my_listen(philosopher);
            }
        }

        if (right_fork->heldBy != id){
            printNTabs(id);
            printf("%d: requesting right fork\n", id);

            MPI_Send(&id, 1, MPI_INT, right_neighbor, 0, MPI_COMM_WORLD);

            while (right_fork->heldBy != id){
                my_listen(philosopher);
            }
        }
    }
}

int main(int argc, char **argv) {
    // Initialize the MPI environment
    MPI_Init(NULL, NULL);

    int world_rank = 5;
    int world_size = 5;

    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    srand(time(NULL) * world_rank);

    int left_neighbor = getLeftIndex(world_rank, world_size);
    int right_neighbor = getRightIndex(world_rank, world_size);

    struct Fork left_fork;
    struct Fork right_fork;

    if (world_rank == 0) {
        left_fork.heldBy = world_rank;
        right_fork.heldBy = world_rank;
        left_fork.dirty = true;
        right_fork.dirty = true;
    }

    if (world_rank != world_size - 1) {
        right_fork.heldBy = world_rank;
        right_fork.dirty = true;
    }

    bool req = false;
    bool req2 = false;
    struct Philosopher p;
    struct Philosopher *philosopher = &p;
    philosopher->id = world_rank;
    philosopher->right_neighbor_id = right_neighbor;
    philosopher->left_neighbor_id = left_neighbor;
    philosopher->right_fork = &right_fork;
    philosopher->left_fork = &left_fork;
    philosopher->left_requested = &req;
    philosopher->right_requested = &req2;

    printNTabs(world_rank);
    printf("%d: ready\n", philosopher->id);

    for (int i = 0; i < 10; i++) {
        think(philosopher);

        getForks(philosopher);

        // Philosopher eats if they have both forks
        int randomTime = rand() % 5 + 1;
        printNTabs(world_rank);
        printf("%d: eating\n", world_rank);
        sleep(randomTime);

        // after eating, both forks become dirty
        left_fork.dirty = true;
        right_fork.dirty = true;
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}


