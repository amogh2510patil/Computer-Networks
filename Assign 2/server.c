#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>


pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

int num_players = 0;
int Game_ID = 0;

// Reads an int from a client socket.
int recv_int(int client_sockfd)
{
    int msg = 0;
    int n = read(client_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) /* Not what we were expecting. Client likely disconnected. */
        return -1;
    return msg;
}

// Writes a message to a client socket. 
void write_client_msg(int client_sockfd, char * msg)
{
    write(client_sockfd, msg, strlen(msg));
}

// Writes an int to a client socket. 
void write_client_int(int client_sockfd, int msg)
{
    write(client_sockfd, &msg, sizeof(int));
}

// Listener socket configuration
int listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening listener socket.");
    // Zero out the memory for the server information
    memset(&serv_addr, 0, sizeof(serv_addr));
	// Server config
    serv_addr.sin_family = AF_INET;	
    serv_addr.sin_addr.s_addr = INADDR_ANY;	
    serv_addr.sin_port = htons(portno);		

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding listener socket.");

    /* Return the socket number. */
    return sockfd;
}

// Sets up the client sockets and client connections
void get_clients(int server_sockfd, int * client_sockfd)
{
    socklen_t client_len;
    struct sockaddr_in serv_addr, client_addr;

    // Listen for two clients
    int player_num = 0;
    while(player_num < 2)
    {
        // Listen for clients
	    listen(server_sockfd, 20);
        memset(&client_addr, 0, sizeof(client_addr));

        client_len = sizeof(client_addr);
	    // Accepting the connection
        client_sockfd[player_num] = accept(server_sockfd, (struct sockaddr *) &client_addr, &client_len);
    
        if (client_sockfd[player_num] < 0)
            error("ERROR accepting a connection from a client.");
        
        // Sending client player ID
        write(client_sockfd[player_num], &player_num, sizeof(int));
        
        pthread_mutex_lock(&mutexcount);
        num_players++;
        printf("Current Number of players %d.\n", num_players);
        pthread_mutex_unlock(&mutexcount);

        // Player 1 is informed to wait until another player has arrived
        if (player_num == 0) {
            write_client_msg(client_sockfd[0],"HLD");
        }
        player_num++;
    }
}

/* Gets a move from a client. */
int get_player_move(int client_sockfd)
{
    /* Tell player to make a move. */
    write_client_msg(client_sockfd, "TRN");

    /* Get players move. */
    return recv_int(client_sockfd);
}

//Figure out if the players want to replay or not
int check_replay(int client_sockfd1, int client_sockfd2)
{   
    int replay = 0;

    write_client_msg(client_sockfd1, "REP");
    write_client_msg(client_sockfd2, "REP");
    int replay_1 = recv_int(client_sockfd1);
    int replay_2 = recv_int(client_sockfd2);

    //Both players want to replay
    if (replay_1 == 1 && replay_2 == 1)
    {
        replay = 1;
        write_client_msg(client_sockfd1, "CLN");
        write_client_msg(client_sockfd2, "CLN");
        printf("Playing Another Game \n");
    }
    //Either one player wishes to play
    if (replay_1 == 1 && replay_2 == 0)
    {
        replay = 0;
        printf("Player 2 wishes to Quit \n");
        write_client_msg(client_sockfd1, "REJ");
        
    }
    if (replay_1 == 0 && replay_2 == 1)
    {
        replay = 0;
        printf("Player 1 wishes to Quit \n");
        write_client_msg(client_sockfd2, "REJ");
        
    }
    return replay;
}
/* Checks that a players move is valid. */
int check_move(char game_board[][3], int move, int player_id)
{   
    //Unoccupied - Move is valid
    if (game_board[move/3][move%3] == ' ') { /* Move is valid. */
             
        return 1;
   }
   //Move is invalid
   else { 
       return 0;
   }
}

/* Updates the board with a new move. */
void board_update(char game_board[][3], int move, int player_id)
{
    game_board[move/3][move%3] = player_id ? 'X' : 'O';
}

/* Sends a board update to both clients. */
void send_update(int * client_sockfd, int move, int player_id, int game_ID)
{

    /* Signal an update */    
    write_client_msg(client_sockfd[0], "UPD");
    write_client_msg(client_sockfd[1], "UPD");

    /* Send the id of the player that made the move. */
    write_client_int(client_sockfd[0], player_id);
    write_client_int(client_sockfd[1], player_id);
    
    /* Send the move. */
    write_client_int(client_sockfd[0], move);
    write_client_int(client_sockfd[1], move);

    /* Send Game ID */
    write_client_int(client_sockfd[0], game_ID);
    write_client_int(client_sockfd[1], game_ID);
}

/* Checks the board to determine if there is a winner. */
int check_board(char game_board[][3], int last_move)
{

    int row = last_move/3;
    int col = last_move%3;

    if ( game_board[row][0] == game_board[row][1] && game_board[row][1] == game_board[row][2] ) { /* Check the row for a win. */

        return 1;
    }
    else if ( game_board[0][col] == game_board[1][col] && game_board[1][col] == game_board[2][col] ) { /* Check the column for a win. */        return 1;
    }
    else if (!(last_move % 2)) { /* If the last move was at an even numbered position we have to check the diagonal(s) as well. */
        if ( (last_move == 0 || last_move == 4 || last_move == 8) && (game_board[1][1] == game_board[0][0] && game_board[1][1] == game_board[2][2]) ) {  /* Check backslash diagonal. */

            return 1;
        }
        if ( (last_move == 2 || last_move == 4 || last_move == 6) && (game_board[1][1] == game_board[0][2] && game_board[1][1] == game_board[2][0]) ) { /* Check frontslash diagonal. */

            return 1;
        }
    }

    /* No winner, yet. */
    return 0;
}

/* Runs a game between two clients. */
void *TicTacToe(void *thread_data) 
{
    int *client_sockfd = (int*)thread_data; /* Client sockets. */
    int id_game = Game_ID;

    char filename[30];
    FILE *fp;
    sprintf(filename,"data%d.log",id_game);
    fp = fopen(filename,"w");
    
    // Clients are informed about the beginning of the game
    write_client_msg(client_sockfd[0], "STR");
    write_client_msg(client_sockfd[1], "STR");
    int replay = 0; //Variable which keeps track if clients want to replay the Game

    //Loop for multiple game Runs
    do {
        printf("Game %d on!\n",id_game);
        replay = 0;
        int prev_player = 1;
        int player_turn = 0;
        int flag_game_complete = 0;
        int num_turns = 0;
        int move = 0;
        int valid_move = 0;

        fprintf(fp,"Game-ID : %d\n", id_game);

        //Board
        char game_board[3][3] = { {' ', ' ', ' '}, 
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };
        
        while(!flag_game_complete) {
            /* Tell other player to wait */
            if (prev_player != player_turn)
                write_client_msg(client_sockfd[(player_turn + 1) % 2], "WAT");

            valid_move = 0;
            move = 0;
            //Keep requesting input from client until the player's move is valid
            while(!valid_move) { 
                move = get_player_move(client_sockfd[player_turn]);
                //If client has disconnected or Timed out
                if (move == -1 || move == -2) {
                    break; 
                }
                printf("Player %d played position %d %d\n", player_turn+1, move/3+1,move%3+1);
                
                //Checking validity of move
                valid_move = check_move(game_board, move, player_turn);
                if (!valid_move) { /* Move was invalid. */
                    printf("Invalid Move.\n");
                    write_client_msg(client_sockfd[player_turn], "INV");
                }
                else {
                    fprintf(fp,"Player %d played position %d %d\n", player_turn+1, move/3+1,move%3+1);
                }
            }

            //If a client has timed out
            if (move == -2) {
                printf("Player %d Took too long to respond \n",player_turn+1);
                fprintf(fp,"Player %d has Timed Out", player_turn + 1);
                write_client_msg(client_sockfd[(player_turn + 1) % 2], "OUT");
                break;
            }
            else if (move == -1) { 
                printf("Player disconnected.\n");
                fprintf(fp,"Player disconnected.\n");
                break;
            }
            else {
                // Update the board and send the update. 
                board_update(game_board, move, player_turn);
                send_update( client_sockfd, move, player_turn, id_game);

                // Check for a winner/loser. 
                flag_game_complete = check_board(game_board, move);
                
                if (flag_game_complete == 1) { // We have a winner. 
                    write_client_msg(client_sockfd[player_turn], "WIN");
                    write_client_msg(client_sockfd[(player_turn + 1) % 2], "LSE");
                    printf("Player %d won.\n", player_turn);
                    fprintf(fp,"Player %d won.\n", player_turn+1);
                }
                else if (num_turns == 8) { // There have been nine valid moves and no winner, game is a draw. 
                    printf("Draw.\n");
                    fprintf(fp,"Draw.\n");
                    write_client_msg(client_sockfd[0], "DRW");
                    write_client_msg(client_sockfd[1], "DRW");
                    flag_game_complete = 1;
                }

                // Move to next player. 
                prev_player = player_turn;
                player_turn = (player_turn + 1) % 2;
                num_turns++;
            }
        }

        printf("Game over.\n");
        replay = check_replay(client_sockfd[0],client_sockfd[1]);
        if (replay && move==-2){
            Game_ID += 1;
            id_game = Game_ID;
        }
    } while (replay == 1);

    fclose(fp);

	/* Close client sockets and decrement player counter. */
    close(client_sockfd[0]);
    close(client_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    num_players=num_players-2;
    printf("Number of players is now %d.", num_players);
    pthread_mutex_unlock(&mutexcount);
    
    free(client_sockfd);

    pthread_exit(NULL);
}

/* Main Program */

int main(int argc, char *argv[])
{   
    /* Check if Port number was mentioned */
    if (argc < 2) {
        fprintf(stderr,"No port was mentioned\n");
        exit(1);
    }
    
    int server_sockfd = listener(atoi(argv[1])); /* Server socket. */
    pthread_mutex_init(&mutexcount, NULL);
    int True = 1;
    while (True) { 
        int *client_sockfd = (int*)malloc(2*sizeof(int)); /* Client sockets */
        memset(client_sockfd, 0, 2*sizeof(int));
        
        /* Get two clients connected. */
        get_clients(server_sockfd, client_sockfd);
        
        /* Start a new thread when two clients have established a connection. */
        pthread_t thread; 
        int result = pthread_create(&thread, NULL, TicTacToe, (void *)client_sockfd); 
        Game_ID += 1;
    }
    close(server_sockfd);
    pthread_mutex_destroy(&mutexcount);
    pthread_exit(NULL); 
}
