#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

const int TASK_NOT_WORKED_ON = -1;
const int TASK_BEING_EXECUTED = -2;
const int TASK_DONE = -3;

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

int checkEndOfGame(struct Board *board, int column, bool debug) {
    return checkEnd(board, column, debug);
}

float evaluate(struct Board *board, bool cpuMove, int lastColumn, int depth, int rank) {
    float result = (float) checkEndOfGame(board, lastColumn, false);
    if (result != (float) STATE_NEUTRAL) {
        if (result == (float) STATE_CPU_WIN) {
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

    float sum = 0;
    bool allChildrenLoss = true;
    bool allChildrenWin = true;
    for (int column = 1; column <= width; column++){
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

        sum += (float) result;
    }

    if (allChildrenWin){
        return 1;
    }

    if (allChildrenLoss){
        return -1;
    }

    return sum / 7;
}

int main(int argc, char **argv){
    struct Board b;
    struct Board *board = &b;

    for (int i = height - 1; i >= 0; i--){
        for (int j = 0; j < width; j++){
            board->spaces[i][j] = 0;
        }
    }

    board->spaces[0][0] = 2;
    board->spaces[1][0] = 1;
    printBoard(board);

    float test = evaluate(board, true, 0, DEPTH_WORKER, 1);
    printf("%f", test);
}
