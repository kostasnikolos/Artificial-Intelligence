#include "global.h"
#include "board.h"
#include "move.h"
#include "comm.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>  
#include <string.h>  



#define MAX_CHILDREN 300   	// max number of available moves at each positio 
#define ΜΑΧ_DEPTH 3     	// max depth allowed for minimax algo


Position gamePosition;     
Move moveReceived;         
Move myMove;               
char myColor;              
int mySocket;              
char msg;

// available names to change depending on the algorithm used
static const char *algorithmNames[3] = {
    "SimpleMinimax",
    "AΒMinimax",
    "AΒOrdering"
};
char agentName[50] = "MultiMinimaxPlayer";

// variable to choose the algo given by the user
int algorithmChoice = 0;

// variables to measure execution time, for each lagorithm usedthere is a slot
static double totalTimeAlg[3] = {0.0, 0.0, 0.0};
static int    moveCountAlg[3] = {0,   0,   0};

/**
 * 	Struct treeNode used for searching in a tree form 
 * 	It represents a certain state of a game given the position on the hextable,
 *  the last move led to it, the next available moves
 * 
 */

typedef struct treeNode {
    Position pos;                				// current position in the table
    Move lastMove;               				// last move that brought us here
    int valuation;               				// value of the certain node
    struct treeNode *children[MAX_CHILDREN];	// next available states (nodes)
    int childCount;								// number of children of current node
} treeNode;

treeNode* createTreeNode(const Position *p, const Move *move);
void freeTree(treeNode *node);
int evaluatePosition(const Position *pos, char maximizingColor);
int expandNode(treeNode *node, char currentColor);
int simpleMinimax(treeNode *node, int depth, char maximizingColor);
int alphaBetaMinimax(treeNode *node, int depth, int alpha, int beta, char maximizingColor);
int alphaBetaMinimaxWithOrdering(treeNode *node, int depth, int alpha, int beta, char maximizingColor);
Move findBestMove(Position *rootPos, char myCol, int alg);

treeNode* createTreeNode(const Position *p, const Move *move)

/**
 * Create a new node given the position and the move that led to it
 */
{	
	
	// create new node
	treeNode* node = (treeNode*) malloc(sizeof(treeNode));

	// save the position given
    memcpy(&node->pos, p, sizeof(Position));  

	// check if this node is a root
    if (move != NULL) {
		// if not save the move
        node->lastMove = *move;
    } else {
		// if yes set last move to NULL
        node->lastMove.tile[0] = NULL_MOVE;
        node->lastMove.color   = p->turn; 
    }

	// set vals to zero
    node->valuation = 0;
    node->childCount = 0;
    for (int i = 0; i < MAX_CHILDREN; i++)
        node->children[i] = NULL;

    return node;
}
/**
 * Free the tree
 */
void freeTree(treeNode *node)
{
    if (!node) return;
    for (int i = 0; i < node->childCount; i++) {
        if (node->children[i]) {
            freeTree(node->children[i]);
            node->children[i] = NULL;
        }
    }
    free(node);
}

/**
 * Check if a given position is terminal by checking if there are any legal moves remaining
 * Basicaly check if  can move is false  for both players collors
 */
int isTerminalPosition(const Position *position)
{
    if (!canMove((Position*)position, WHITE) && !canMove((Position*)position, BLACK)) {
        return 1;
    }
    return 0;
}

/**
 * Simple evaluation of position as given in exercise's decsription
 * we evaluate a certain position by the difference of our and opponet's score
 */
int evaluatePosition(const Position *position, char ourColor)
{	
	// get my score
    int myScore  = position->score[(int) ourColor];
	// get opponets score
    int oppScore = position->score[(int) getOtherSide(ourColor)];
    return (myScore - oppScore);
}


/**
 * Basically scans every empty space in the board and checks for legal moves 
 * If a move is legal we create a new node  representing the new state
 * In order to create the new state we save the current position in the new node and perform the needed move
 * 
 * Returns the number of children created for the given node
 */
int expandNode(treeNode *node, char currentColor)
{
	// save the number of childen created
    int count = 0;
    // Scan every space in the table
    for (int i = 0; i < ARRAY_BOARD_SIZE; i++) {
        for (int j = 0; j < ARRAY_BOARD_SIZE; j++) {
			// check if the position is empty
            if (node->pos.board[i][j] == EMPTY) {
				//  create a temporary  move that will lead to it
                Move move;
                move.tile[0] = i;
                move.tile[1] = j;
                move.color   = currentColor;

				// check if the move is legal
                if ( isLegalMove((Position*)&node->pos, &move) ) {
                    // if yes create a new position with the current position and the move given
                    Position newPos;
                    memcpy(&newPos, &node->pos, sizeof(Position));
                    doMove(&newPos, &move);  

                    // create new child (new node state)
                    treeNode *child = createTreeNode(&newPos, &move);

                    // connect the new child to the current node
                    node->children[node->childCount++] = child;

					// keep track of the children number created
                    count++;

               
                }
            }
        }
    }
    return count;
}


/**
 * Simple Minimax algo
 * The first call is always the maximizer 
 */
int simpleMinimax(treeNode *node, int depth, char maximizingColor)
{
	// define curent player
    char currentPlayer = node->pos.turn;

    // check if a position is terminal 
	// if yes return its value
    if (depth == 0 || isTerminalPosition(&node->pos)) {
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

	// expand the nodes childrens
    int childCount = expandNode(node, currentPlayer);

	// check if there are no available moves (children)
    if (childCount == 0) {  
		// check if its not  a terminal position
        if (!isTerminalPosition(&node->pos)) { 
			// in case its not a terminal position and no available moves 
			// we have to simulate the other players turn since we lose ours
			// create a temporary position switching the turn to the other player
            Position passPos;
            memcpy(&passPos, &node->pos, sizeof(Position));
            passPos.turn = getOtherSide(passPos.turn);
            
            // create a temporary node to call a new minimax for our next turn
            treeNode *tempNode = createTreeNode(&passPos, NULL);

			// call minimax to find oure next turns value
			node->valuation = simpleMinimax(tempNode, depth-1, maximizingColor);

			freeTree(tempNode);
			
            return node-> valuation;
        }
        // if its a terminal position just return is valuation
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

	// case of max player
    if (currentPlayer == maximizingColor) {
        
		// set best val to -oo
        int bestVal = INT_MIN;
		// call Minimax for every child recursively
        for (int i = 0; i < node->childCount; i++) {
			
            treeNode *child = node->children[i];
            int value = simpleMinimax(child, depth-1, maximizingColor);

			// update best value found
            if (value > bestVal) {
                bestVal = value;
    		}
        }
        node->valuation = bestVal;
        return bestVal;
	
    } 
	// case of min player
	else {
        int bestVal = INT_MAX;
        for (int i = 0; i < node->childCount; i++) {

            treeNode *child = node->children[i];
            int value = simpleMinimax(child, depth-1, maximizingColor);
            if (value < bestVal) {
                bestVal = value;
            }
        }
        node->valuation = bestVal;
        return bestVal;
    }
}

/**
 * Minimax algo with alpa-beta prunning.
 * Difference is we do not call minimax algo for every child recursively
 * Instead we keep values a,b update them and
 * if we find a value that we know it wont pe picked by the opponent 
 * we cut the remaining childs of the node 
 */
int alphaBetaMinimax(treeNode *node, int depth, int alpha, int beta, char maximizingColor)
{	
	// define curent player
    char currentPlayer = node->pos.turn;

	// check if a position is terminal 
	// if yes return its value
    if (depth == 0 || isTerminalPosition(&node->pos)) {
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

	// expand the nodes childrens
    int childCount = expandNode(node, currentPlayer);
	// check if there are no available moves (children)
    if (childCount == 0) {
        // check if its not  a terminal position
        if (!isTerminalPosition(&node->pos)) {
			// in case its not a terminal position and no available moves 
			// we have to simulate the other players turn since we lose ours
			// create a temporary position switching the turn to the other player
            Position passPos;
            memcpy(&passPos, &node->pos, sizeof(Position));
            passPos.turn = getOtherSide(passPos.turn);

			// create a temporary node to call a new minimax for our next turn
            treeNode *tempNode = createTreeNode(&passPos, NULL);

			// call minimax to find oure next turns value
			node->valuation = alphaBetaMinimax(tempNode, depth-1, alpha, beta, maximizingColor);

			freeTree(tempNode);

			return node->valuation;
        }
        // if its a terminal position just return is valuation
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

	// case of max player
    if (currentPlayer == maximizingColor) {
		//  set best val to -oo
        int bestVal = INT_MIN;
		// call a-b pruning for  every child recursively until we stop when we find a smaller value of beta
        for (int i=0; i<node->childCount; i++) {
            treeNode *child = node->children[i];
            int value = alphaBetaMinimax(child, depth-1, alpha, beta, maximizingColor);
            if (value > bestVal) {
                bestVal = value;
            }
            if (value > alpha) {
                alpha = value;
            }
            // no need to search anymore cause we want select this case
            if (beta <= alpha) {
                break;
            }
        }
        node->valuation = bestVal;
        return bestVal;
    } 
	// case of min player
	else {
        int bestVal = INT_MAX;
        for (int i=0; i<node->childCount; i++) {
            treeNode *child = node->children[i];
            int value = alphaBetaMinimax(child, depth-1, alpha, beta, maximizingColor);
            if (value < bestVal) {
                bestVal = value;
            }
            if (value < beta) {
                beta = value;
            }
            // no need to search anymore cause we want select this case
            if (beta <= alpha) {
                break;
            }
        }
        node->valuation = bestVal;
        return bestVal;
    }
}

/**
 * A simple yet effective change on the classic a-b pruning.
 * Before we perform the a-b pruning algo we order the childrens of the node accordingly to promote pruning.
 * For a max player we move first the childrens with the biggest  valuation
 * For a min player we move first the childrens with the lowest valuation
 * 
 */
int alphaBetaMinimaxWithOrdering(treeNode *node, int depth, int alpha, int beta, char maximizingColor)
{
    // define curent player
    char currentPlayer = node->pos.turn;

	// check if a position is terminal 
	// if yes return its value
    if (depth == 0 || isTerminalPosition(&node->pos)) {
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

	// expand the nodes childrens
    int childCount = expandNode(node, currentPlayer);
	// check if there are no available moves (children)
    if (childCount == 0) {
        // check if its not  a terminal position
        if (!isTerminalPosition(&node->pos)) {
			// in case its not a terminal position and no available moves 
			// we have to simulate the other players turn since we lose ours
			// create a temporary position switching the turn to the other player
            Position passPos;
            memcpy(&passPos, &node->pos, sizeof(Position));
            passPos.turn = getOtherSide(passPos.turn);

			// create a temporary node to call a new minimax for our next turn
            treeNode *tempNode = createTreeNode(&passPos, NULL);

			// call minimax to find oure next turns value
			node->valuation = alphaBetaMinimaxWithOrdering(tempNode, depth-1, alpha, beta, maximizingColor);

			freeTree(tempNode);

			return node->valuation;
        }
        // if its a terminal position just return is valuation
        node->valuation = evaluatePosition(&node->pos, maximizingColor);
        return node->valuation;
    }

    //  Move Ordering 
    // first calculate the valuation for each children
    for (int i=0; i<node->childCount; i++) {
        treeNode *c = node->children[i];
        c->valuation = evaluatePosition(&c->pos, maximizingColor);
    }
    // order chids with decreasing order to promote pruning
    if (currentPlayer == maximizingColor) {
        for (int i=0; i<node->childCount-1; i++) {
            for (int j=i+1; j<node->childCount; j++) {
                if (node->children[i]->valuation < node->children[j]->valuation) {
                    treeNode* tmp = node->children[i];
                    node->children[i] = node->children[j];
                    node->children[j] = tmp;
                }
            }
        }
    }
    // order childs with increasing order to promote prunng
    else {
        for (int i=0; i<node->childCount-1; i++) {
            for (int j=i+1; j<node->childCount; j++) {
                if (node->children[i]->valuation > node->children[j]->valuation) {
                    treeNode* tmp = node->children[i];
                    node->children[i] = node->children[j];
                    node->children[j] = tmp;
                }
            }
        }
    }

    // case of max player
    if (currentPlayer == maximizingColor) {
        //  set best val to -oo
        int bestVal = INT_MIN;
        // call a-b pruning for  every child recursively until we stop when we find a smaller value of beta
        for (int i=0; i<node->childCount; i++) {
            treeNode *child = node->children[i];
            int value = alphaBetaMinimaxWithOrdering(child, depth-1, alpha, beta, maximizingColor);
            if (value > bestVal) bestVal = value;
            if (value > alpha) alpha = value;
            // no need to search anymore cause we want select this case
            if (beta <= alpha) {
                break;
            }
        }
        node->valuation = bestVal;
        return bestVal;
    }
    // case of min player
    else {
        int bestVal = INT_MAX;
        for (int i=0; i<node->childCount; i++) {
            treeNode *child = node->children[i];
            int value = alphaBetaMinimaxWithOrdering(child, depth-1, alpha, beta, maximizingColor);
            if (value < bestVal) bestVal = value;
            if (value < beta) beta = value;
            // κλάδεμα
            if (beta <= alpha) {
                break;
            }
        }
        node->valuation = bestVal;
        return bestVal;
    }
}



Move findBestMove(Position *rootPos, char myCol, int alg)
{
    treeNode* root = createTreeNode(rootPos, NULL);

    int bestVal = 0;
    switch (alg) {
        case 0:
            bestVal = simpleMinimax(root, ΜΑΧ_DEPTH, myCol);
            break;
        case 1:
            bestVal = alphaBetaMinimax(root, ΜΑΧ_DEPTH, INT_MIN, INT_MAX, myCol);
            break;
        case 2:
            bestVal = alphaBetaMinimaxWithOrdering(root, ΜΑΧ_DEPTH, INT_MIN, INT_MAX, myCol);
            break;
        default:
            bestVal = alphaBetaMinimax(root, ΜΑΧ_DEPTH, INT_MIN, INT_MAX, myCol);
            break;
    }

    // now we have to find the child that led to the best valuation calculated
    Move bestMove;
    bestMove.tile[0] = NULL_MOVE;

    // first check it they were no available moves so return NULL
    if (root->childCount == 0) {
        freeTree(root);
        return bestMove;
    }


    // search every children to find the one with the best value calculated 
    for (int i=0; i < root->childCount; i++) {
        if (root->children[i]->valuation == bestVal) {
            bestMove = root->children[i]->lastMove;
            break;
        }
    }

    freeTree(root);
    return bestMove;
}



int main(int argc, char **argv)
{
    int c;
    opterr = 0;

    char *ip = "127.0.0.1";
    char *port = "6002";

    while( ( c = getopt ( argc, argv, "i:p:a:h" ) ) != -1 )
    {
        switch( c )
        {
            case 'h':
                printf( "Usage: %s [-i server-ip] [-p server-port]\n", argv[0] );
                printf("   -a  : which algorithm to use?\n");
                printf("       0 => Simple Minimax (no alpha-beta)\n");
                printf("       1 => Alpha-Beta Minimax\n");
                printf("       2 => Alpha-Beta with Move Ordering\n");
                return 0;
            case 'i':
                ip = optarg;
                break;
            case 'p':
                port = optarg;
                break;
			case 'a':
                algorithmChoice = atoi(optarg);
                if (algorithmChoice < 0 || algorithmChoice > 2) {
					printf(" you should have selected a number from { 0,1,2} ");
                    algorithmChoice = 0; // default
                }
                break;
            case '?':
                if( optopt == 'i' || optopt == 'p' )
                    printf( "Option -%c requires an argument.\n", ( char ) optopt );
                else if( isprint( optopt ) )
                    printf( "Unknown option -%c\n", ( char ) optopt );
                else
                    printf( "Unknown option character `\\x%x`.\n", optopt );
                return 1;
            default:
                return 1;
        }
    }

    // set agent name that we will use given the algo the user gave us
    strcpy(agentName, algorithmNames[algorithmChoice]);

    connectToTarget(port, ip, &mySocket);
    srand(time(NULL));

    while (1)
    {
        msg = recvMsg(mySocket);

        switch (msg)
        {
            case NM_REQUEST_NAME:
                sendName(agentName, mySocket);
                break;

            case NM_NEW_POSITION:
                getPosition(&gamePosition, mySocket);
                printPosition(&gamePosition);
                break;

            case NM_COLOR_W:
                myColor = WHITE;
                break;

            case NM_COLOR_B:
                myColor = BLACK;
                break;

            case NM_PREPARE_TO_RECEIVE_MOVE:
                getMove(&moveReceived, mySocket);
                moveReceived.color = getOtherSide(myColor);
                doMove(&gamePosition, &moveReceived);
                printPosition(&gamePosition);
                break;

            case NM_REQUEST_MOVE:
            {
                myMove.color = myColor;
                
                // keep track of time
                clock_t startTime = clock();
                
                // no available moves so return null
                if (!canMove(&gamePosition, myColor)) {
                    myMove.tile[0] = NULL_MOVE;
                } else {
                    myMove = findBestMove(&gamePosition, myColor, algorithmChoice);

                    // fallback to a random move if we cannot find a legal one
                    if (myMove.tile[0] != NULL_MOVE && !isLegalMove(&gamePosition, &myMove)) {
                        fprintf(stderr, "WARNING: returned illegal move. Fallback to random.\n");
                        int found=0;
                        for (;;) {
                            int rr = rand() % ARRAY_BOARD_SIZE;
                            int cc = rand() % ARRAY_BOARD_SIZE;
                            if (gamePosition.board[rr][cc] == EMPTY) {
                                myMove.tile[0] = rr;
                                myMove.tile[1] = cc;
                                if ( isLegalMove(&gamePosition, &myMove) ) {
                                    found=1;
                                    break;
                                }
                            }
                        }
                        if (!found) {
                            myMove.tile[0] = NULL_MOVE;
                        }
                    }
                }

                clock_t endTime = clock();
                double elapsedSec = (double)(endTime - startTime) / CLOCKS_PER_SEC;

                // update time and move counters
                totalTimeAlg[algorithmChoice] += elapsedSec;
                moveCountAlg[algorithmChoice]++;

                sendMove(&myMove, mySocket);
                doMove(&gamePosition, &myMove);
                printPosition(&gamePosition);

                // printf(" Move time: %.3f sec\n", elapsedSec);
                break;
            }

            case NM_QUIT:


                printf("\n--- Game finished. Statistics ---\n");
                for (int alg=0; alg<3; alg++) {
                    if (moveCountAlg[alg] > 0) {
                        double avg = totalTimeAlg[alg] / (double)moveCountAlg[alg];
                        printf("Algorithm %s -> moves: %d, average time: %.4f s\n",
                            algorithmNames[alg],
                            moveCountAlg[alg],
                            avg);
                    }
                }


                close(mySocket);
                return 0;
        }
    }

    close(mySocket);
    return 0;
}
