/**
 * Contains functions to manage user's password
 */

#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H


#include <stdbool.h>


#define MAX_USERNAME_LEN 63
#define MAX_PASSWORD_LEN 63


/**
 * Initalize this service on server
 */
void initialize_authentication_service();


/**
 * Check if the password for the given username is correct
 * @return true if the username with the given password exist, else false
 */
bool check_user(const char* username, const char* password);


/**
 * Associate the password to the username
 * @return false if fail (user already exist), else true
 */
bool create_user(const char* username, const char* password);


#endif // AUTH_SERVICE_H