#ifndef STORAGE_SERVICE_H_
#define STORAGE_SERVICE_H_


#include "FileChecksum.h"


#define MAX_FILE_NAME_LEN 64 // this includes null-terminator


/**
 * Define a linked list node to contain all file infos
 */
struct FileInfo {
	char name[MAX_FILE_NAME_LEN];
	uint32_t checksum;
	struct FileInfo* next;
};


/**
 * Initialize this service on server
 */
void initialize_storage_service();


/**
 * Create the directory to store user's file
 * @return 0 if success, -1 if fail
 */
int create_user_directory(const char* username);


/**
 * Find the info of all files of a given user
 * @param  username  Name of user
 * @param  n_files   [out] Number of files found
 * @return Linked list of file infos.
 *         The memory for the linked list is dynamically allocated,
 *         so user must call free_file_info on the returned pointer
 *         after finishes using the linked list.
 */
struct FileInfo* list_user_files(const char* username, int* n_files);


/**
 * Find the info of all files in the given directory
 * @param  dir_path  path to directory
 * @param  n_files   [out] Number of files found
 * @return Pointer to a FileInfo struct, which represents the
 *         head of a linked list of FileInfo.
 *         The memory for the linked list is dynamically allocated,
 *         so user must call free_file_info on the returned pointer
 *         after finishes using the linked list.
 */
struct FileInfo* list_files(const char* dir_path, int* n_files);


/**
 * Free the dynamically allocated list of file info
 */
void free_file_info(struct FileInfo* file_info_list);


/**
 * @return A dynamically allocated string representing the path <p1>/<p2>
 */
char* join_path(const char* p1, const char* p2);


/**
 * @return A string representing the path to an user directory
 *         The string is dynamically allocated, and needed to be
 *         freed afterward.
 */
char* path_to_user(const char* username);


#endif // STORAGE_SERVICE_H_
