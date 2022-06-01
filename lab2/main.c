#include <mpi.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

const int TASK_NOT_WORKED_ON = -2;
const int TASK_BEING_EXECUTED = -3;
const int TASK_DONE = -4;

const int DEPTH_ALL = 8;
const int DEPTH_MASTER = 3;
const int DEPTH_WORKER = DEPTH_ALL - DEPTH_MASTER;

const int STATE_NEUTRAL = 0;
const int STATE_PLAYER_WIN = 1;
const int STATE_CPU_WIN = 2;

const int SPACE_EMPTY = 0;
const int SPACE_PLAYER = 1;
const int SPACE_CPU = 2;

const int REQUEST_DATA = 1;
const int REQUEST_INITIAL = 2;
const int REQUEST_TASK = 3;
const int REQUEST_RESULT = 4;
const int REQUEST_WAIT = 5;
const int REQUEST_END = 6;

const int width = 7;
const int height = 6;
const int max_message = width * height + 1;

const int DIM_TASKS = width;
const int MAX_TASKS = DIM_TASKS * DIM_TASKS;

// region board
struct Board {
    int state;
    int spaces[height][width];
};

struct Board *doMove(struct Board *board, int spacePlayer, int column) {
    if (column > 7 || column < 1) {
        return board;
    }

    int i = height - 1;
    while (i >= 0) {
        if (board->spaces[i][column - 1] != 0) {
            board->spaces[i + 1][column - 1] = spacePlayer;
            return board;
        }
        i -= 1;
    }

    board->spaces[0][column - 1] = spacePlayer;
    return board;
}

void printBoard(struct Board *board) {
    for (int i = 1; i < width + 1; i++) {
        printf(" %d ", i);
    }

    printf("\n");
    printf("=============================\n");

    for (int i = height - 1; i >= 0; i--) {
        for (int y = 0; y < width; y++) {
            printf(" %d ", board->spaces[i][y]);
        }
        printf("\n");
    }
}

struct Board *undoMove(struct Board *board, int column){
    if (column > 7 || column < 1){
        printf("column not in range [1,7]\n");
        return board;
    }

    for (int i = height - 1; i >= 0; i--){
        if (board->spaces[i][column - 1] != SPACE_EMPTY){
            board->spaces[i][column - 1] = SPACE_EMPTY;
            return board;
        }
    }

    return board;
}

int checkEnd(struct Board *board, int last_move, bool debug) {
    int indexWidth = last_move - 1;

    if (debug){
        printf("checkEnd with debug\n");
    }

    if (board->spaces[0][indexWidth] == SPACE_EMPTY) {
        if (debug){
            printf("error in check returning board\n");
        }
        return STATE_NEUTRAL;
    }

    if (debug){
        printf("checkEnd with debug above indexHeight = height - 1\n");
    }

    int indexHeight = height - 1;
    for (; indexHeight >= 0; indexHeight--) {
        if (board->spaces[indexHeight][indexWidth] != SPACE_EMPTY) {
            break;
        }
    }

    if (debug){
        printf("checkEnd with debug above spaceSign init\n");
    }

    int spaceSign = board->spaces[indexHeight][indexWidth];

    // go left
    int leftMostIndex = indexWidth;
    for (; leftMostIndex >= 0; leftMostIndex--) {
        if (board->spaces[indexHeight][leftMostIndex] != spaceSign) {
            break;
        }
    }

    leftMostIndex = leftMostIndex + 1;

    // go right
    int rightMostIndex = indexWidth;
    for (; rightMostIndex < width; rightMostIndex++) {
        if (board->spaces[indexHeight][rightMostIndex] != spaceSign) {
            break;
        }
    }

    rightMostIndex = rightMostIndex - 1;

    if (debug){
        printf("checkEnd with debug above if (rightMostIndex - leftMostIndex == 3)\n");
    }

    if (rightMostIndex - leftMostIndex == 3) {
        if (debug){
            printf("left to right solution rightMostIndex: %d, leftMostIndex: %d\n", rightMostIndex, leftMostIndex);
        }
        return spaceSign;
    }

    // go up
    int upMostIndex = indexHeight;
    for (; upMostIndex >= 0; upMostIndex--) {
        if (board->spaces[upMostIndex][indexWidth] != spaceSign) {
            break;
        }
    }

    upMostIndex = upMostIndex + 1;

    // go down
    int downMostIndex = indexHeight;
    for (; downMostIndex < height; downMostIndex++) {
        if (board->spaces[downMostIndex][indexWidth] != spaceSign) {
            break;
        }
    }

    downMostIndex = downMostIndex - 1;

    if (debug){
        printf("checkEnd with debug above if downMostIndex - upMostIndex == 3\n");
    }
    if (downMostIndex - upMostIndex == 3) {
        if (debug){
            printf("down to up solution downMostIndex: %d, upMostIndex: %d\n", downMostIndex, upMostIndex);
        }
        return spaceSign;
    }

    // go left-up
    int leftUpDiff = 0;
    for (; indexHeight - leftUpDiff >= 0 && indexWidth - leftUpDiff >= 0; leftUpDiff++) {
        if (board->spaces[indexHeight - leftUpDiff][indexWidth - leftUpDiff] != spaceSign) {
            break;
        }
    }

    leftUpDiff--;

    // go right-down
    int rightDownDiff = 0;
    for (; indexHeight + rightDownDiff < height && indexWidth + rightDownDiff < width; rightDownDiff++) {
        if (board->spaces[indexHeight + rightDownDiff][indexWidth + rightDownDiff] != spaceSign) {
            break;
        }
    }

    rightDownDiff--;

    if (debug){
        printf("checkEnd with debug above leftUpDiff + rightDownDiff + 1 == 4\n");
    }
    if (leftUpDiff + rightDownDiff + 1 == 4) {
        if (debug){
            printf("leftup to righdown solution leftUpDiff: %d, rightDownDiff: %d\n", leftUpDiff, rightDownDiff);
        }
        return spaceSign;
    }

    // go left-down
    int leftDownDiff = 0;
    for (; indexHeight + leftDownDiff < height && indexWidth - leftDownDiff >= 0; leftDownDiff++) {
        if (board->spaces[indexHeight + leftDownDiff][indexWidth - leftDownDiff] != spaceSign) {
            break;
        }
    }

    leftDownDiff--;

    // go right-down
    int rightUpDiff = 0;
    for (; indexHeight - rightUpDiff >= 0 && indexWidth + rightUpDiff < width; rightUpDiff++) {
        if (board->spaces[indexHeight - rightUpDiff][indexWidth + rightUpDiff] != spaceSign) {
            break;
        }
    }

    rightUpDiff--;

    if (debug){
        printf("checkEnd with debug above leftDownDiff + rightUpDiff + 1 == 4\n");
    }
    if (leftDownDiff + rightUpDiff + 1 == 4) {
        if (debug){
            printf("leftdown to rightup solution leftDownDiff: %d, rightUpDiff: %d\n", leftDownDiff, rightUpDiff);
        }
        return spaceSign;
    }

    if (debug){printf("checkEnd returning board with state %d\n", board->state);}

    return STATE_NEUTRAL;
}

// endregion board

int userInput() {
    while (true) {
        printf("Column: ");
        int column;
        scanf("%d", &column);

        if (column >= 0 && column <= width) {
            return column;
        }

        printf("0 for end or 1 through 7 for column number");
    }
}

int checkEndOfGame(struct Board *board, int column, bool debug) {
    return checkEnd(board, column, debug);
}

int checkEndOfBoard(struct Board *board) {
    int result;

    for (int i = 1; i <= width; i++) {
        result = checkEndOfGame(board, i, false);
        if (result != STATE_NEUTRAL) {
            return result;
        }
    }
    return STATE_NEUTRAL;
}

double calculateQualityOfMove(double tasks[], int x) {
    double sum = 0;
    for (int y = 0; y <= DIM_TASKS; y++) {
        double curr = tasks[y * DIM_TASKS + x];
        sum += curr;
    }

    return sum / 7;
}

void sendEndToAll(int world_size) {
    double buf[max_message];
    buf[0] = (double) REQUEST_END;
//    int number = REQUEST_END;
    for (int i = 1; i < world_size; i++) {
        MPI_Send(buf, max_message, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
    }
}

int getATask(double tasks[], int length) {
    for (int i = 0; i < length; i++) {
        if (tasks[i] == (double) TASK_NOT_WORKED_ON) {
            printf("returning task %d\n", i);
            return i;
        }
    }
    return -1;
}

void doMain(int world_size, int rank) {
    struct Board b;
    struct Board *board = &b;
    board->state = STATE_NEUTRAL;
    for (int y = height - 1; y >= 0; y--){
        for (int x = 0; x < width; x++){
            board->spaces[y][x] = SPACE_EMPTY;
        }
    }

    int result = checkEndOfBoard(board);
    if (result != STATE_NEUTRAL) {
        sendEndToAll(world_size);
        printf("Winner is: ");
        if (result == STATE_CPU_WIN) {
            printf("computer\n");
        } else {
            printf("player\n");
        }

        return;
    }

    printf("MAIN: entering while loop\n");
    while (true) {
        time_t now = time(0);

        double tasks[MAX_TASKS];
        for (int i = 0; i < MAX_TASKS; i++) {
            tasks[i] = (double) TASK_NOT_WORKED_ON;
        }

        printf("MAIN: about to send board data\n");
        for (int i = 1; i < world_size; i++) {
            double buf[max_message];
            buf[0] = (double) REQUEST_DATA;
            for (int y = height - 1; y >= 0; y--) {
                for (int x = 0; x < width; x++) {
                    buf[y * width + x + 1] = (double) board->spaces[y][x];
                }
            }

            printf("MAIN: sending board to %d\n", i);
            MPI_Send(buf, max_message, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
        }

        int active_workers = world_size - 1;
        bool done = false;
        printf("MASTER: entering while !done loop\n");
        while (!done) {
            double buf[max_message];
            MPI_Status status;
            printf("MASTER: receiving message ");
            MPI_Recv(buf, max_message, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

            int sourceIndex = status.MPI_SOURCE;

            printf("MASTER: received from %d ", sourceIndex);
            switch((int) buf[0]){
                case REQUEST_RESULT:
                    printf("REQUEST_RESULT\n");
                    break;
                case REQUEST_END:
                    printf("REQUEST_END\n");
                    break;
                case REQUEST_DATA:
                    printf("REQUEST_DATA\n");
                    break;
                case REQUEST_INITIAL:
                    printf("REQUEST_INITIAL\n");
                    break;
                case REQUEST_WAIT:
                    printf("REQUEST_WAIT\n");
                    break;
                case REQUEST_TASK:
                    printf("REQUEST_TASK\n");
                    break;
                default:
                    printf("UNKNOWN\n");
            }

            if (buf[0] == (double) REQUEST_INITIAL) {
                double send_buf[max_message];
//                printf("tasks: ");
//                for (int i = 0; i < MAX_TASKS; i++){
//                    printf(" %f ", tasks[i]);
//                }
//                printf("\n");

                int taskId = getATask(tasks, MAX_TASKS);
                if (taskId != -1) {
                    tasks[taskId] = (double) TASK_BEING_EXECUTED;
//                    printf("tasks[%d] = %d\n", taskId, TASK_BEING_EXECUTED);
                    send_buf[0] = (double) REQUEST_TASK;
                    send_buf[1] = (double) taskId;
                    printf("MASTER: sending REQUEST_TASK to %d\n", sourceIndex);
                    MPI_Send(send_buf, max_message, MPI_DOUBLE, sourceIndex, 0, MPI_COMM_WORLD);
                } else {
                    active_workers--;
                    if (active_workers == 0) {
                        done = true;
                    }
                    send_buf[0] = (double) REQUEST_WAIT;
                    MPI_Send(send_buf, max_message, MPI_DOUBLE, sourceIndex, 0, MPI_COMM_WORLD);
                }
            }

            if (buf[0] == (double) REQUEST_RESULT) {
                tasks[(int) buf[1]] = buf[2];
            }
        }

        double qualities[DIM_TASKS];
        for (int x = 0; x <= DIM_TASKS; x++) {
            qualities[x] = calculateQualityOfMove(tasks, x);
        }

        int indexMax = -1;
        double max = -1;
        for (int i = 0; i < DIM_TASKS; i++) {
            if (max == -1 || max < qualities[i]) {
                max = qualities[i];
                indexMax = i;
            }
        }

        time_t end = time(0);

        for (int x = 0; x < DIM_TASKS; x++) {
            printf("quality of move %d is %f\n", x, qualities[x]);
        }
        printf("best column: %d\n", indexMax);
        printf("move was calculated in: %f seconds\n", difftime(end, now));

        printf("MASTER: doing best move on column: %d\n", indexMax);
        board = doMove(board, SPACE_CPU, indexMax + 1);
        printBoard(board);
        result = checkEndOfGame(board, indexMax, false);
        if (result != STATE_NEUTRAL) {
            printf("CPU is winner\n");
            sendEndToAll(world_size);
            break;
        }

        int player_move = userInput();
        if (player_move == 0) {
            printf("Terminating\n");
            sendEndToAll(world_size);
            break;
        } else {
            board = doMove(board, SPACE_PLAYER, player_move);
            printBoard(board);
            result = checkEndOfGame(board, player_move, false);
            if (result != STATE_NEUTRAL) {
                if (result == STATE_CPU_WIN) {
                    printf("ERROR: CPU won on player turn");
                }

                printf("Player is the winner\n");
                sendEndToAll(world_size);
                break;
            }
        }
    }
}

double evaluate(struct Board *board, bool cpuMove, int lastColumn, int depth, int rank) {
    double result = (double) checkEndOfGame(board, lastColumn, false);
    if (result != (double) STATE_NEUTRAL) {
        if (result == (double) STATE_CPU_WIN) {
            return 1;
        } else {
            return -1;
        }
    }

    if (depth == 0){
        return 0;
    }

    int label;
    if (cpuMove){
        label = STATE_CPU_WIN;
    } else {
        label = STATE_PLAYER_WIN;
    }

    double sum = 0;
    bool allChildrenLoss = true;
    bool allChildrenWin = true;
    for (int column = 1; column <= width; column++){
        if (board->spaces[height - 1][column - 1] != SPACE_EMPTY){
            continue;
        }
        board = doMove(board, label, column);
        result = evaluate(board, !cpuMove, column, depth - 1, rank);
        board = undoMove(board, column);

        if (result > -1){
            allChildrenLoss = false;
        }

        if (result != 1){
            allChildrenWin = false;
        }

        if (result == 1 && !cpuMove){
            return 1;
        }

        if (result == -1 && cpuMove){
            return -1;
        }

        sum += (double) result;
    }

    if (allChildrenWin){
        return 1;
    }

    if (allChildrenLoss){
        return -1;
    }

    return sum / 7;
}

void doSlave(int rank) {
    struct Board b;
    struct Board *board = &b;
    b.state = STATE_NEUTRAL;
    while (true){
        double buf[max_message];
        printf("%d slave receiving message\n", rank);
        MPI_Recv(buf, max_message, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        if (buf[0] == (double) REQUEST_END){
            sleep(1);
            break;
        }

        if (buf[0] == (double) REQUEST_DATA){
            for (int y = height - 1; y >= 0; y--){
                for (int x = 0; x < width; x++){
                    board->spaces[y][x] = (int) buf[1 + y * width + x];
                }
            }
        }
//        printf("slave %d board: ", rank);
//        printBoard(board);

        bool done = false;
        while (true){
            double send_buf[max_message];
            send_buf[0] = (double) REQUEST_INITIAL;

            printf("slave %d sending REQUEST_INITIAL\n", rank);
            MPI_Send(send_buf, max_message, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);

            MPI_Recv(buf, max_message, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (buf[0] == (double) REQUEST_WAIT){
                break;
            }

            int task = (int) buf[1];
            int cpu_move = task / 7 + 1;
            int player_move = task % 7 + 1;
//            printf("%d got task = %d. Cpu move = %d, player move = %d\n", rank, task, cpu_move, player_move);

            board = doMove(board, SPACE_CPU, cpu_move);

//            printf("%d board after cpu move", rank);
//            printBoard(board);
//
//            printf("\n");

//            int test = checkEndOfGame(board, cpu_move, false);
//            printf("%d endOfGame after cpu_move: %d\n", rank, test);
            if (checkEndOfGame(board, cpu_move, false) != STATE_NEUTRAL){
                printf("%d entered cpu_win IF\n", rank);
                int result = 1;
                board = undoMove(board, cpu_move);
                send_buf[0] = (double) REQUEST_RESULT;
                send_buf[1] = (double) task;
                send_buf[2] = (double) result;
                MPI_Send(send_buf, max_message, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
                continue;
            }

            board = doMove(board, SPACE_PLAYER, player_move);
//            printf("%d board after player move", rank);
//            printBoard(board);
//
//            printf("\n");

//            test = checkEndOfGame(board, player_move, false);
//            printf("%d endOfGame after player_move: %d\n", rank, test);
            if (checkEndOfGame(board, player_move, false) != STATE_NEUTRAL){
                int result = -1;
                board = undoMove(board, player_move);
                board = undoMove(board, cpu_move);
                send_buf[0] = (double) REQUEST_RESULT;
                send_buf[1] = (double) task;
                send_buf[2] = (double) result;
                MPI_Send(send_buf, max_message, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
                continue;
            }

            printf("%d evaluating\n", rank);
            double result = evaluate(board, true, player_move, DEPTH_WORKER, rank);
            printf("%d exited evaluate\n", rank);

            board = undoMove(board, player_move);
            board = undoMove(board, cpu_move);

            send_buf[0] = (double) REQUEST_RESULT;
            send_buf[1] = (double) task;
            send_buf[2] = result;

            printf("%d sending REQUEST_RESULT for task %d with result %f", rank, task, result);
            MPI_Send(send_buf, max_message, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
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

    if (world_rank == 0) {
        printf("master starting\n");
        doMain(world_size, world_rank);
    } else {
        printf("slave %d starting\n", world_rank);
        doSlave(world_rank);
    }

    // Finalize the MPI environment.
    MPI_Finalize();
}