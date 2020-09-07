#include "Protocol.h"

#include <arpa/inet.h>  /* htons, ntohs */
#include <stdio.h>      /* file IO */
#include <string.h>     /* memcpy */


/**
 * Read from TCP connection until the number of bytes read is at least the target specified
 * @param  socket      TCP Socket to read from
 * @param  buffer      Buffer to read into
 * @param  buff_len    Maximum length of the buffer
 * @param  n_received  Number of bytes already received before this call
 * @param  target_len  The least number of bytes to be received in total
 * @return Number of bytes read in total, or -1 if error
 */
ssize_t receive_packet_until(int socket, char* buffer, size_t buff_len, int n_received, int target_len) {
    while(n_received < target_len) {
        int n_new_bytes = recv(socket, buffer + n_received, 
                               buff_len - n_received, 0);
        if (n_new_bytes <= 0) {
            // fail to recv
            return -1;
        }
        n_received += n_new_bytes;
    }
    return n_received;
}


ssize_t receive_packet(int socket ,char* buffer, size_t buff_len) {
    int n_received = 0;
    // since TCP doesn't have message boundary, we have to make sure that
    // the packet is received fully. In order to do this, we first read 
    // the full packet header, which contains the packet length, then
    // use that length as stopping condition for the rest of the loop.

    // receive the header, and find the packet length
    n_received = receive_packet_until(socket, buffer, buff_len, 0, HEADER_LEN);
    if (n_received <= 0) {
        // failed to receive data
        return -1;
    }
    struct PacketHeader* header = (struct PacketHeader*)buffer;
    int packet_len = ntohs(header->packet_len);
    
    // don't receive entire packet if it's too large
    if (packet_len > buff_len) {
        packet_len = buff_len;
    }

    // receive the rest of the packet
    n_received = receive_packet_until(socket, buffer, buff_len, n_received, packet_len);
    // wrong packet length
    if (n_received != packet_len) {
        return -1;
    }
    return packet_len;
}


/**
 * Helper function to write packet header 
 */
void make_header(char* buffer, enum PacketType type, uint16_t packet_len, uint32_t token) {
    struct PacketHeader* header = (struct PacketHeader*) buffer;
    header->version = VERSION;
    header->type = type;
    header->packet_len = htons(packet_len);
    header->session_token = token;
}


ssize_t make_header_only_packet(char* buffer, size_t buff_len, enum PacketType type, uint32_t token) {
    /* make sure buffer has enough length */
    if (buff_len < HEADER_LEN) {
        return -1;
    }
    make_header(buffer, type, HEADER_LEN, token);
    return HEADER_LEN;
}


ssize_t make_logon_request(char* buffer, 
                     size_t buff_len, 
                     bool is_new_account,
                     const char* username, 
                     const char* password) {

    /* make sure buffer has enough length */
    
    size_t user_len = strlen(username) + 1; // include null terminator
    size_t pass_len = strlen(password) + 1; // include null terminator
    size_t packet_len = HEADER_LEN + user_len + pass_len;
    // if buffer too small, return with error
    if (buff_len < packet_len) {
        return -1;
    }

    /* write header */

    enum PacketType type = is_new_account? TYPE_SIGNUP_REQUEST : TYPE_LOGON_REQUEST;
    make_header(buffer, type, packet_len, 0);

    /* write data */

    // write user name (including null terminator)
    buffer += HEADER_LEN;
    memcpy(buffer, username, user_len);

    // write password (including null terminator)
    buffer += user_len;
    memcpy(buffer, password, pass_len);
    return packet_len;
}


ssize_t make_token_response(char* buffer, size_t buff_len, uint32_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_TOKEN_RESPONSE, token);
}


ssize_t make_leave_request(char* buffer, size_t buff_len, uint32_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LEAVE_REQUEST, token);
}


ssize_t make_list_request(char* buffer, size_t buff_len, uint32_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_LIST_REQUEST, token);
}


ssize_t make_list_response(char* buffer, size_t buff_len, uint32_t token, 
        struct FileInfo* file_info, int n_files) {
    // make sure buffer is big enough for packet
    size_t packet_len = HEADER_LEN + (MAX_FILE_NAME_LEN + 4) * n_files;
    if (buff_len < packet_len) {
        return -1;
    }

    // write header
    make_header(buffer, TYPE_LIST_RESPONSE, packet_len, token);
    buffer += HEADER_LEN;

    // write data
    int i;
    for (i = 0; i < n_files; i++) {
        // file name
        memcpy(buffer, file_info->name, MAX_FILE_NAME_LEN);
        buffer += MAX_FILE_NAME_LEN;
        // 4-byte checksum
        uint32_t checksum_network_endian = htonl(file_info->checksum);
        memcpy(buffer, &checksum_network_endian, 4);
        buffer += 4;
        file_info = file_info->next;
    }

    return packet_len;
}


ssize_t make_file_request(
        char* buffer, size_t buff_len, uint32_t token, const char* file_name) {
    size_t file_name_len = strlen(file_name) + 1;  // include null terminator
    size_t packet_len = HEADER_LEN + file_name_len;

    // make sure buffer is big enough for packet
    if (buff_len < packet_len) {
        return -1;
    }

    // write header and data
    make_header(buffer, TYPE_FILE_REQUEST, packet_len, token);
    memcpy(buffer + HEADER_LEN, file_name, file_name_len);
    return packet_len;
}


ssize_t make_file_transfer_header(char* buffer, size_t buff_len, uint32_t token, uint16_t data_len) {
    if (buff_len < HEADER_LEN) {
        return -1;
    }
    make_header(buffer, TYPE_FILE_TRANSFER, HEADER_LEN + data_len, token);
    return HEADER_LEN;
}


ssize_t make_file_transfer_body(char* buffer, size_t buff_len, FILE* file) {
    return fread(buffer, 1, buff_len, file);
}


ssize_t make_file_received_packet(char* buffer, size_t buff_len, uint32_t token) {
    return make_header_only_packet(buffer, buff_len, TYPE_FILE_RECEIVED, token);
}


ssize_t make_error_response(char* buffer, size_t buff_len, uint32_t token, enum ErrorType error) {
    size_t packet_len = HEADER_LEN + 1;
    if (buff_len < packet_len) {
        return -1;
    }
    make_header(buffer, TYPE_ERROR, packet_len, token);
    buffer[HEADER_LEN] = error;
    return packet_len;
}


