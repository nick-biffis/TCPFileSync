/**
 * The username - password hash pair are stored in a file, where each entry 
 * of the file contains an username and the corresponding password hash.
 * The username is padded with 0 until MAX_USERNAME_LEN + 1 (so it is always
 * null terminated), then the following 4 bytes are password hash
 */

#include "AuthenticationService.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "md5.h"


// MAX_USERNAME_LEN (63+1) + 16-byte hash
#define MAX_LINE_LEN 80
// hash will be 16 bytes
#define HASH_LEN 16
#define DATABASE_DIR  "serverdata"
#define DATABASE_FILE "serverdata/password.dat"




void initialize_authentication_service() {
	// simply create the folder to store data
	mkdir(DATABASE_DIR, 0777);
}


void hash_password(const char* password, unsigned char* result) {
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, password, strlen(password));
	MD5_Final(result, &context);
}


bool compare_hash(unsigned char* hash, unsigned char* correct_hash) {
	int i;
	for (i = 0; i < HASH_LEN; i++) {
		if (hash[i] != correct_hash[i]) {
			return false;
		}
	}
	return true;
}


bool check_user(const char* username, const char* password) {
	size_t username_len = strlen(username);
	size_t password_len = strlen(password);
	if (username_len <= 0 || username_len > MAX_USERNAME_LEN
			|| password_len <= 0 || password_len > MAX_PASSWORD_LEN) {
		return false; 
	}

	unsigned char hash[HASH_LEN];
	hash_password(password, hash);

	// check for username and password in database
	char cur_line[MAX_LINE_LEN];

	FILE* db_file = fopen(DATABASE_FILE, "rb");
	if (db_file == NULL) {
		return false;
	}

	unsigned char correct_hash[HASH_LEN];
	while (fread(cur_line, 1, MAX_LINE_LEN, db_file) == MAX_LINE_LEN) {
		if (strcmp(cur_line, username) == 0) {
			memcpy(correct_hash, cur_line + MAX_USERNAME_LEN + 1, HASH_LEN);
			fclose(db_file);
			return compare_hash(hash, correct_hash);
		}
	}
	fclose(db_file);
	return false;
}


bool create_user(const char* username, const char* password) {
	size_t username_len = strlen(username);
	size_t password_len = strlen(password);
	if (username_len <= 0 || username_len > MAX_USERNAME_LEN
			|| password_len <= 0 || password_len > MAX_PASSWORD_LEN) {
		return false; 
	}
	
	unsigned char hash[HASH_LEN];
	hash_password(password, hash);

	// make sure username doesn't already exist
	// if so add the username and hash to database
	FILE* db_file = fopen(DATABASE_FILE, "a+b");
	char cur_line[MAX_LINE_LEN];

	while (fread(cur_line, 1, MAX_LINE_LEN, db_file) == MAX_LINE_LEN) {
		if (strcmp(cur_line, username) == 0) {
			// username already exist
			fclose(db_file);
			return false;
		}
	}
	// zero out line
	memset(cur_line, 0, MAX_LINE_LEN);
	// start with username (null terminated)
	memcpy(cur_line, username, username_len + 1);
	// append hash
	memcpy(cur_line + MAX_USERNAME_LEN + 1, &hash, HASH_LEN);
	fwrite(cur_line, 1, MAX_LINE_LEN, db_file);
	fclose(db_file);
	return true;
}