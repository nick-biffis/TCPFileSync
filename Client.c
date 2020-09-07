/**
 * GetMyMusic client's main program
 */

#include <stdbool.h>
#include <sys/stat.h>

#include "AuthenticationService.h"
#include "NetworkHeader.h"
#include "Protocol.h"
#include "StorageService.h"


#define CLIENT_DIR "clientdata"


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
void parse_arguments(int argc, char* argv[], char** server, char** port);


/**
 * Create a TCP socket connecting to the server at specified port
 * If an error happens, log error message and exit the program.
 * Return the socket file descriptor
 *
 * @param server      Server IP or server hostname
 * @param server_port Server port number (as a string)
 */
int create_socket(const char* server, const char* server_port);


/**
 * Query the server for the list of files belong to the user
 *
 * @param  server_socket Server socket
 * @param  buffer        Buffer to receive packet
 * @param  session_token Session token of current user
 * @param  n_files       [out] Address of variable to store number of files
 * @return Linked list of file infos at server. This is dynamically allocated,
 *         and require a call to free_file_info() to release memory.
 */
struct FileInfo* get_server_files(int server_socket, char* buffer, uint32_t session_token, int* n_files);


/**
 * Return a linked list of files that appear in src but doesn't appear in dst.
 * The criteria for file equality is checksum
 * This method allocate memory for the returned list, while not changing the
 * original lists.
 */
struct FileInfo* get_missing_files(struct FileInfo* src, struct FileInfo* dst);


/**
 * Get the files that only exist in client or in server. The 2 lists returned
 * are dynamically allocated, and must be freed using free_file_info()
 *
 * @param  server_socket   Server socket
 * @param  buffer          Buffer to receive packet
 * @param  session_token   Session token of current user
 * @param  client_missings [out] Address of variable to store list of files
 *                         missing from client
 * @param  server_missings [out] Address of variable to store list of files
 *                         missing from server
 */
void get_client_server_diffs(int server_socket, char* buffer, uint32_t session_token, 
        struct FileInfo** client_missings, struct FileInfo** server_missings);


/**
 * Prompt the user to input a number between 1 and max_option (inclusive)
 * @return The option chosen by user
 */
int get_input(const char* prompt, int max_option);


/*
 * Handler of each client command
 */


/**
 * Prompt for username and password, and send logon/signup request
 * to server. Die if error happens.
 * @return Session token for this user
 */
uint32_t handle_logon(int server_socket, char* buffer);


void handle_list(int server_socket, char* buffer, uint32_t session_token);


void handle_diff(int server_socket, char* buffer, uint32_t session_token);


void handle_sync(int server_socket, char* buffer, uint32_t session_token);


/**
 * Excecute the client
 */
int main (int argc, char *argv[]) {

    /*
     * Parse arguments supplied to main program
     */

    char* server = SERVER_HOST; // init with default value
    char* port = SERVER_PORT;   // init with default value
    parse_arguments(argc, argv, &server, &port);

    /*
     * Initialize socket and IO buffers
     */
    
    int server_socket = create_socket(server, port);
    char buffer[BUFFSIZE];

    /*
     * Initialize database
     */
    mkdir(CLIENT_DIR, 0777);


    /*
     * Logon/sign-up
     */
    uint32_t session_token = handle_logon(server_socket, buffer);

    /*
     * Handle user's commands
     */

    bool quit = false;
    while(!quit) {
        printf("\n========================\n");
        int choice = get_input(
                "Select command:\n  1. List server files\n  2. Diff\n  3. Sync\n  4. Quit",
                 4);
        printf("\n");
        switch(choice) {
            case 1:
                // list file from server
                handle_list(server_socket, buffer, session_token);
                break;
            case 2:
                // diff between client and server files
                handle_diff(server_socket, buffer, session_token);
                break;
            case 3:
                // sync server and client
                handle_sync(server_socket, buffer, session_token);
                break;
            default:
                quit = true;
                break;
        }
    }

    // Request to leave
    ssize_t packet_len = make_leave_request(buffer, BUFFSIZE, session_token);
    send(server_socket, buffer, packet_len, 0);

    // Release resource and exit
    close(server_socket);
    return 0;
}


/*
 * Function implementations
 */


void die_with_error(const char* message, const char* detail) {
    printf("ERROR: %s\n", message);
    if (detail != NULL) { 
        printf("       %s\n", detail);
    }
    exit(1);
}


void parse_arguments(int argc, char* argv[], char** server, char** port) {
    static const char* USAGE_MESSAGE = 
            "Usage:\n ./client [-h <server>] [-p <port>]";
    
    // there must be an odd number of arguments (program name and flag-value pairs)
    if (argc % 2 == 0 || argc > 5) {
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
            case 'h':  // server IP or hostname
                *server = value;
                break;
            case 'p':  // server port string
                *port = value;
                break;
            default:   // unknown flag
                die_with_error(USAGE_MESSAGE, "Unknown flag");
        }
    }
}


int create_socket(const char* server, const char* server_port) {
    /*
     * define criteria for address info
     */
    struct addrinfo addr_criteria;                    // struct storing the address info
    memset(&addr_criteria, 0, sizeof(addr_criteria)); // zero out the struct
    addr_criteria.ai_family = AF_UNSPEC;              // any address family
    addr_criteria.ai_socktype = SOCK_STREAM;          // only accept stream socket
    addr_criteria.ai_protocol = IPPROTO_TCP;          // only use TCP protocol      

    /* 
     * get a list of address infos that fit the criteria
     */
    struct addrinfo* server_addr; // the start of a linked list of possible addresses
    int err_code = getaddrinfo(server, server_port, &addr_criteria, &server_addr);
    if (err_code != 0) {
        die_with_error("Cannot get server's address info - getaddrinfo() failed", gai_strerror(err_code));
    }

    /*
     * try all addresses in the list of address infos
     * until successful connection is established
     */
    int server_socket = -1;
    struct addrinfo* addr;
    for (addr = server_addr; addr != NULL; addr = addr->ai_next) {
        // create socket from address info
        server_socket = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (server_socket < 0) {
            continue; // fail to create socket, try other addresses
        }
        // initiate connection to server
        if (connect(server_socket, addr->ai_addr, addr->ai_addrlen) == 0) {
          break; // successfully connect to a socket
        }
        // fail to connect, close all resources and try other addresses
        close(server_socket);
        server_socket = -1;
    }

    /*
     * Clean up and return
     */
    // free the dynamically allocated list of address infos
    freeaddrinfo(server_addr);
    // die if cannot successfully connect with server
    if (server_socket < 0) {
        die_with_error("Failed to connect to server", "connect() failed");
    }

    return server_socket;
}


struct FileInfo* get_server_files(int server_socket, char* buffer, uint32_t session_token, int* n_files) {
    // Ask for list of files from server
    ssize_t packet_len = make_list_request(buffer, BUFFSIZE, session_token);
    send(server_socket, buffer, packet_len, 0);

    // receive list of files from server
    packet_len = receive_packet(server_socket, buffer, BUFFSIZE);
    if (packet_len <= 0) {
        printf("Error when receiving list repsonse\n");
        exit(1);
    }

    // parse packet into a list of files
    struct FileInfo* server_files = NULL;
    *n_files = (packet_len - HEADER_LEN) / (MAX_FILE_NAME_LEN+4);
    char* cur_entry = buffer + HEADER_LEN;
    int i;
    for (i = 0; i < *n_files; i++) {
        struct FileInfo* cur_file = malloc(sizeof(struct FileInfo));
        // copy file name
        memcpy(cur_file->name, cur_entry, MAX_FILE_NAME_LEN);
        cur_entry += MAX_FILE_NAME_LEN;
        // copy file checksum (with endian corrected)
        memcpy(&cur_file->checksum, cur_entry, 4);
        cur_file->checksum = ntohl(cur_file->checksum);
        cur_entry += 4;
        // add file to head of server file list
        cur_file->next = server_files;
        server_files = cur_file;
    }
    return server_files;
}


struct FileInfo* get_missing_files(struct FileInfo* src, struct FileInfo* dst) {
    struct FileInfo* cur_src;
    struct FileInfo* cur_dst;
    struct FileInfo* missing = NULL;
    for (cur_src = src; cur_src != NULL; cur_src = cur_src->next) {
        // check if current file in src is found in dst
        bool found = false;
        for (cur_dst = dst; cur_dst != NULL; cur_dst = cur_dst->next) {
            if (cur_src->checksum == cur_dst->checksum) {
                found = true;
                break;
            }
        }
        // if not found, add current file to missing list
        if (!found) {
            // copy the info into new struct
            struct FileInfo* cur_file = malloc(sizeof(struct FileInfo));
            memcpy(cur_file, cur_src, sizeof(struct FileInfo));
            // add to the head of the missing list
            cur_file->next = missing;
            missing = cur_file;
        }
    }
    return missing;
}


void get_client_server_diffs(int server_socket, char* buffer, uint32_t session_token, 
        struct FileInfo** client_missings, struct FileInfo** server_missings) {
    int n_server_files, n_client_files;
    struct FileInfo* server_files = get_server_files(server_socket, buffer, session_token, &n_server_files);
    struct FileInfo* client_files = list_files(CLIENT_DIR, &n_client_files);

    *client_missings = get_missing_files(server_files, client_files);
    *server_missings = get_missing_files(client_files, server_files);

    free_file_info(server_files);
    free_file_info(client_files);
}


void upload_file(int server_socket, char* buffer, uint32_t session_token, const char* file_name) {
    printf("Uploading file %s\n", file_name);
    // open file descriptor
    char* file_path = join_path(CLIENT_DIR, file_name);
    FILE* file = fopen(file_path, "rb");
    free(file_path);
    if (file == NULL) {
        return;
    }
    // get size of file
    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    // send header
    size_t packet_len = make_file_transfer_header(buffer, BUFFSIZE, session_token, MAX_FILE_NAME_LEN + file_size);
    send(server_socket, buffer, packet_len, 0);
    // send file name
    send(server_socket, file_name, MAX_FILE_NAME_LEN, 0);
    // send the entire file
    while ((packet_len = make_file_transfer_body(buffer, BUFFSIZE, file)) > 0) {
        send(server_socket, buffer, packet_len, 0);
    }
    fclose(file);
}


void download_file(int server_socket, char* buffer, uint32_t session_token, const char* file_name) {
    printf("Downloading file %s\n", file_name);
    // request the server to send the file
    size_t packet_len = make_file_request(buffer, BUFFSIZE, session_token, file_name);
    send(server_socket, buffer, packet_len, 0);

    // receive the file back
    size_t n_received = receive_packet(server_socket, buffer, BUFFSIZE);
    struct PacketHeader* header = (struct PacketHeader*) buffer;
    size_t response_len = ntohs(header->packet_len);

    // open a new file to write to
    char* file_path = join_path(CLIENT_DIR, file_name);
    FILE* file = fopen(file_path, "wb");

    // write the file content to file
    fwrite(buffer + HEADER_LEN, 1, n_received - HEADER_LEN, file);

    // continue to receive more file content and write to file
    while(n_received < response_len) {
        int n_new_bytes = recv(server_socket, buffer, BUFFSIZE, 0);
        if (n_new_bytes <= 0) {
            // fail to recv
            fclose(file);
            remove(file_path);        
            free(file_path);
            return;
        }
        n_received += n_new_bytes;
        fwrite(buffer, 1, n_new_bytes, file);
    }

    fclose(file);
    free(file_path);
}


int get_input(const char* prompt, int max_option) {
    static char input[BUFFSIZE];
    // repeatedly prompt for input, until read a valid input
    while (true) {
        printf("%s\n", prompt);
        printf(">> ");
        fgets(input, BUFFSIZE, stdin);
        int choice = atoi(input);
        if (choice > 0 && choice <= max_option) {
            return choice;
        }
    }
}


uint32_t handle_logon(int server_socket, char* buffer) {
    // Prompt for username and password
    int choice = get_input("Logon or signup?\n  1. Logon\n  2. Sign up", 2);
    bool is_new_user = (choice == 2);
    printf("\nEnter username: ");
    char username[MAX_USERNAME_LEN];
    scanf("%s", username);
    char* password = getpass("Enter password: ");
    fgets(buffer, BUFFSIZE, stdin); // consume new line character

    // Create logon request
    ssize_t packet_len = make_logon_request(buffer, BUFFSIZE, is_new_user, username, password);
    send(server_socket, buffer, packet_len, 0);

    // Receive a session token
    packet_len = receive_packet(server_socket, buffer, BUFFSIZE);
    if (packet_len <= 0) {
        die_with_error("Failed to login/signup", NULL);
    }

    struct PacketHeader* header = (struct PacketHeader*) buffer;
    uint32_t session_token = header->session_token;
    
    // Check for error
    if (header->type == TYPE_ERROR) {
        enum ErrorType error = buffer[HEADER_LEN] & 0xFF;
        const char* err_msg = "Failed to login";
        if (error == ERROR_SERVER_BUSY) {
            die_with_error(err_msg, "Server busy");
        } else if (error == ERROR_USERNAME_TAKEN) {
            die_with_error(err_msg, "Username already existed");
        } else if (error == ERROR_INVALID_PASSWORD) {
            die_with_error(err_msg, "Invalid username or password");
        } else {
            die_with_error(err_msg, "Unknown error");
        }
        // never reached
    }

    printf("\nWelcome, %s!\n", username);
    return session_token;
}


void handle_list(int server_socket, char* buffer, uint32_t session_token) {
    int n_files;
    struct FileInfo* server_files = get_server_files(server_socket, buffer, session_token, &n_files);
    printf("Found %d files on server\n", n_files);
    if (n_files == 0) {
        return;        
    }
    printf("%-32s%8s\n", "File name", "Checksum");
    struct FileInfo* cur_file;
    // print all file infos in the linked list
    for (cur_file = server_files; cur_file != NULL; cur_file = cur_file->next) {
        printf("%-32s%8x\n", cur_file->name, cur_file->checksum);
    }
    free_file_info(server_files);
}


void handle_diff(int server_socket, char* buffer, uint32_t session_token) {
    // get the diffs of server and client's files
    struct FileInfo* client_missings;
    struct FileInfo* server_missings;
    get_client_server_diffs(server_socket, buffer, session_token, &client_missings, &server_missings);

    // print the list of missing files
    printf("Files not in client:\n");
    struct FileInfo* cur_file;
    for (cur_file = client_missings; cur_file != NULL; cur_file = cur_file->next) {
        printf("  %s\n", cur_file->name);
    }
    printf("\nFiles not in server:\n");
    for (cur_file = server_missings; cur_file != NULL; cur_file = cur_file->next) {
        printf("  %s\n", cur_file->name);
    }

    // release dynamically allocated resources
    free_file_info(server_missings);
    free_file_info(client_missings);
}


void handle_sync(int server_socket, char* buffer, uint32_t session_token) {
    // get the diffs of server and client's files
    struct FileInfo* client_missings;
    struct FileInfo* server_missings;
    get_client_server_diffs(server_socket, buffer, session_token, &client_missings, &server_missings);

    struct FileInfo* cur_file;
    // upload to server the missing files
    for (cur_file = server_missings; cur_file != NULL; cur_file = cur_file->next) {
        // send file to server
        upload_file(server_socket, buffer, session_token, cur_file->name);
        // receive confirmation from server
        receive_packet(server_socket, buffer, BUFFSIZE);
    }

    for (cur_file = client_missings; cur_file != NULL; cur_file = cur_file->next) {
        // download from server
        download_file(server_socket, buffer, session_token, cur_file->name);
    }

    free_file_info(client_missings);
    free_file_info(server_missings);
    printf("Sync completed\n");
}
