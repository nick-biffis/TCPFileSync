/**
 * Contains functions to handle client requests
 */

#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_


#include <stdint.h>

#define USERNAME_LEN 128
#define USERNAME_LEN_WITH_NULL 129
#define MAX_CONNECTIONS 64


/**
 * Contains the session info of a currently connected client
 */
struct ClientInfo {
	int client_socket;
	char username[USERNAME_LEN_WITH_NULL];
	uint32_t session_token;
};


/**
 * Initialize
 */
void initialize_client_handler();


/**
 * Accept a new client connection. The client is rejected if number of current connections
 * already reached max number allowed.
 * Also set up book-keeping data for the new client.
 * @param server_socekt   Server socket
 * @param client_infos    Array of slots to store client info for each connected client
 * @param max_connections Max number of connections allowed
 */ 
void accept_client(int server_socket, struct ClientInfo* client_infos, int max_connections);


/**
 * Handle a client request, and update the client info if needed
 */
void handle_client(struct ClientInfo* client_info);

#endif // CLIENT_HANDLER_H_