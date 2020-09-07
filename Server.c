/**
 * Run a server
 */

#include <time.h>  // for setting random seed

#include "NetworkHeader.h"
#include "ClientHandler.h"


/**
 * Print out the error, then exit the program
 * detail can be NULL, in which case no additional detail is printed
 *
 * @param message Error message to be printed out
 * @param detail  Details about the error, can be NULL
 */
void die_with_error(const char* message, const char* detail);


/**
 * Parse the argument supplied to the main program
 * and extract arguments into output parameters.
 * If arguments are invalid (missing required arguments,
 * invalid flags, etc.) exit the program
 *
 * @param argc        Number of command line arguments
 * @param argv        Array of command line arguments
 * @param server      [out] Address of the variable to store server IP
 * @param port        [out] Address of the variable to store the port string
 */
void parse_arguments(int argc, char* argv[], int* port);


/**
 * Create the server socket at the specified port
 * @return the socket descriptor, or -1 if fail to create socket
 */
int create_socket(int server_port);


int main(int argc, char *argv[])
{
	/*
     * Parse arguments supplied to main program
     */
	int server_port = atoi(SERVER_PORT);  // init with default value
	parse_arguments(argc, argv, &server_port);


	/*
	 * Initialize
	 */
	// set seed for random calls in other services
	srand(time(0));
	int i;
	int server_socket = create_socket(server_port);

 	// keep track of which sockets has incoming data
	fd_set activated_sockets;

	// initial infos about connected clients
	struct ClientInfo client_infos[MAX_CONNECTIONS];
	memset(client_infos, 0, MAX_CONNECTIONS * sizeof(struct ClientInfo));

	// intialize client handler
	initialize_client_handler();

	/*
	 * Do all the work here
	 */
	while (1) {
		/*
		 * add all sockets to monitored set
		 */
		// store the maximum socket descriptor for select()
		int max_descriptor = server_socket;

		// clear the set
		FD_ZERO(&activated_sockets);
		// add server socket into set
		FD_SET(server_socket, &activated_sockets);
		// add all client sockets to set
		for (i = 0; i < MAX_CONNECTIONS; i++) {
			// check for val
			int client_socket = client_infos[i].client_socket;
			if (client_socket > 0) {
				FD_SET(client_socket, &activated_sockets);   
            }
            if(client_socket > max_descriptor) {
                max_descriptor = client_socket;
            }
		}

		/*
		 * Wait for activity on some of the sockets
		 */
		int n_activities = select(max_descriptor + 1, &activated_sockets, NULL, NULL, NULL);
		if (n_activities <= 0) {
			continue;
		}

		/*
		 * Handle activity for each activated socket
		 */
		// connection from new client
		if (FD_ISSET(server_socket, &activated_sockets)) {
			printf("\nHandling connection request\n");				
			accept_client(server_socket, client_infos, MAX_CONNECTIONS);
		}
		// request from connected clients
		for (i = 0; i < MAX_CONNECTIONS; i++) {
			if (FD_ISSET(client_infos[i].client_socket, &activated_sockets)) {
				printf("\nHandling client with client ID = %d\n", i);				
				handle_client(&client_infos[i]);
			}
		}
	}

	// not reached
	close(server_socket);
	return 0;
}


/*
 * Function implementation
 */


void die_with_error(const char* message, const char* detail) {
    printf("Error: %s\n", message);
    if (detail != NULL) { 
        printf("       %s\n", detail);
    }
    exit(1);
}


void parse_arguments(int argc, char* argv[], int* port) {
	static const char* USAGE_MESSAGE = 
            "Usage:\n ./server [-p <port>]";
    
    // there must be an odd number of arguments (program name and flag-value pairs)
    if (argc % 2 == 0 || argc > 3) {
        die_with_error(USAGE_MESSAGE, NULL);
    }

    // argument list must have the form:
    // program_name -flag1 value1 -flag2 value ...
    int i;
    for (i = 1; i < argc; i+= 2) {
        // argv[i] must be a flag
        // argv[i+1] must be the flag's value

        // check for flag
        if (argv[i][0] != '-') {
            die_with_error(USAGE_MESSAGE, NULL);
        }

        // assign value to variable associated with the flag
        char* value = argv[i+1];
        switch (argv[i][1]) {
            case 'p':  // server port
                *port = atoi(value);
                break;
            default:   // unknown flag
                die_with_error(USAGE_MESSAGE, "Unknown flag");
        }
    }
}


int create_socket(int server_port) {
	int server_socket;
	/*
	 * Create the socket descriptor
	 */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		die_with_error("Failed to initialize server", "socket() failed");
	}

	/*
	 * Bind socket to local address
	 */
	// create local address struct
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server_port);

	// bind to local address
	if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr))) {
		die_with_error("Failed to initialize server", "bind() failed");
	}

	/*
	 * Set to listen for multiple incomming connections
	 */
	if (listen(server_socket, MAX_CONNECTIONS) < 0) {
		die_with_error("Failed to initialize server", "listen() failed");
	}

	return server_socket;
}
