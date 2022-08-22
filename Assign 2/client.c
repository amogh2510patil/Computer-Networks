#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    printf("The server shut down or the other player has disconnected.\nGame over.\n");
    exit(0);
}

/* Reads a message from the server socket. */
void recieve_command(int sockfd, char * msg)
{
    /* All messages are 3 bytes. */
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3) /* Not what we were expecting. Server got killed or the other client disconnected. */ 
        error("ERROR reading message from server socket.");
}

/* Reads an int from the server socket. */
int recieve_int(int sockfd)
{
    int val = 0;
    int n = read(sockfd, &val, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");

    return val;
}

/* Writes an int to the server socket. */
void send_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int)); 
}

/* Sets up the connection to the server. */
int connect_to_server(char * hostname, int portno)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
 
    // Create a socket. */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	
    if (sockfd < 0) 
        error("ERROR opening socket for server.");
	
    // Get address of the server. 
    server = gethostbyname(hostname);
	
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
	
	// Zero out memory for server info. 
	memset(&serv_addr, 0, sizeof(serv_addr));

	// Set up the server info. 
    serv_addr.sin_family = AF_INET;
    memmove(server->h_addr, &serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno); 

	// Make the connection.
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting to server");
    return sockfd;
}

// Draws the game board to stdout.
void print_board(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("-----------\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
    printf("\n");
}

// Get's the players turn and sends it to the server.
void take_turn(int sockfd)
{
    char buffer[10];
    fd_set rfds;
    struct timeval tv;
    int retval, len;

    // Watch stdin (fd 0) to see when it has input. 
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);

    // Wait up to five seconds. 
    tv.tv_sec = 15;
    tv.tv_usec = 0;        

    int True = 1;
    while (True) { 
        printf("Enter row col to make a move \n");
        retval = select(1, &rfds, NULL, NULL, &tv);

        if (retval){
            /* Read data from stdin using fgets. */
            fgets(buffer, 10, stdin);
        }
        else {
            printf("\n No data input in 15 seconds.\n");
            send_int(sockfd, -2); 
            break;
        }    
        //Convert the 2-D co-ordinates to a single value ranging from 0-8
	    int ro = buffer[0] - '0';
        int col = buffer[2] - '0';
        int move = (ro-1)*3 + (col-1);
        if (ro <= 0 || col <= 0 || ro>3 || col>3)  {
            printf("\nInvalid input. Try again.\n");
        }
        else if (move < 9 && move >= 0){
            printf("\n");
            // Send move to the server.
            send_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }
}

//Get the player's choices for a re-match
int replay(int sockfd)
{
    char buffer[10];
    int True = 1;
    while (True) {
        printf("Do you wish to play again: Y|N \n");
        fgets(buffer, 10, stdin);
        printf("\n");
        if (buffer[0]=='Y')
        {
            send_int(sockfd, 1);
            return 1;
        }
        else if (buffer[0]=='N')
        {
            send_int(sockfd, 0);
            return 0;
        }
        else
        {
            printf("Invalid Input, Please Re-enter your choice \n");
        }
    }
}

// Gets a board update from the server.
void get_info(int sockfd, char board[][3])
{
    // Get the update. 
    int player_id = recieve_int(sockfd);
    int move = recieve_int(sockfd);
    int game_ID = recieve_int(sockfd);
    
    printf("Game : %d \n",game_ID);
    // Update the game board.
    board[move/3][move%3] = player_id ? 'X' : 'O';    
}

int main(int argc, char *argv[])
{
    // Check if host and port are specified. 
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }

    // Connecting to the server. 
    int sockfd = connect_to_server(argv[1], atoi(argv[2]));

    int id = recieve_int(sockfd);
    char msg[4];
    char board[3][3] = { {' ', ' ', ' '}, /* Game board */
                         {' ', ' ', ' '}, 
                         {' ', ' ', ' '} };

    printf("Tic-Tac-Toe\n------------\n");

    int flag = 0;
    /* Wait for the game to start. */
    do {
        recieve_command(sockfd, msg);
        if (!strcmp(msg, "HLD"))
            printf("Waiting for a second player...\n");
        if (flag==0) {
            printf("\n Player ID: %d \n",id+1);
            flag =1;
        }
    } while ( strcmp(msg, "STR") );

    /* The game has begun. */
    printf("You are %c's\n", id ? 'X' : 'O');
    printf("Starting the Game...\n");
    

    print_board(board);

    int True = 1;
    while(True) {
        recieve_command(sockfd, msg);

        if (!strcmp(msg, "TRN")) { /* Players turn. */
	        printf("Your move...\n");
	        take_turn(sockfd);
        }
        else if (!strcmp(msg, "INV")) { // Move invalid. 
            printf("That position has already been played. Try again.\n"); 
        }
        else if (!strcmp(msg, "CNT")) { /* Server is sending the number of active players. Note that a "TRN" message will always follow a "CNT" message. */
            int num_players = recieve_int(sockfd);
            printf("There are currently %d active players.\n", num_players); 
        }
        else if (!strcmp(msg, "UPD")) { // board update.
            get_info(sockfd, board);
            print_board(board);
        }
        else if (!strcmp(msg, "WAT")) { // Wait for other player to take a turn. 
            printf("Waiting for other players move...\n");
        }
        else if (!strcmp(msg, "WIN")) { // Winner. 
            printf("You win!\n");
        }
        else if (!strcmp(msg, "LSE")) { // Loser.
            printf("You lost!\n");
        }
        else if (!strcmp(msg, "DRW")) { //Draw. 
            printf("Draw.\n");
        }
        else if (!strcmp(msg, "REP")) { //Re-match Decision
            int rep = replay(sockfd);
            if (!rep)
            {
                printf("Thank You for Playing!! \n");
                break;
            }
        }
        else if (!strcmp(msg, "CLN")) {   //Cleaning the board for a new game
            printf("Playing Another Game... \n");
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                        board[i][j] = ' ';
                }
            }
            print_board(board);
        }                   
        else if (!strcmp(msg, "REJ")) {     //When other player refuses rematch
            printf("The Other Player Does not wish to Play \n");
            printf("Thank You For Playing!! \n");
            break;
        }
        else if (!strcmp(msg, "OUT")) {     
            printf("\n The Other Player took to long to respond \n");
        }
        else /* Weird... */
            error("Unknown message.");
    }
    
    /* Close server socket and exit. */
    close(sockfd);
    return 0;
}