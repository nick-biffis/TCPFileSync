#include "StorageService.h"

#include <dirent.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>


#define DATABASE_DIR "serverdata"


/*
 * Helper functions
 */

/**
 * Check if the path refers to a regular file (as opposed to a directory/sym link/etc.)
 */
bool is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}


/*
 * Public functions
 */


void initialize_storage_service() {
	// simply create the folder to store user files
	mkdir(DATABASE_DIR, 0777);
}


int create_user_directory(const char* username) {
	char* user_dir_path = path_to_user(username);
	printf("Create directory %s\n", user_dir_path);
	int success = mkdir(user_dir_path, 0777);
	free(user_dir_path);
	return success;
}


struct FileInfo* list_user_files(const char* username, int* n_files) {
	/*
	 * Construct path to user's directory
	 */
	char* dir_path = path_to_user(username);

	/*
	 * Get a list of all user files
	 */
	struct FileInfo* file_list = list_files(dir_path, n_files);

	free(dir_path);
	return file_list;
}



struct FileInfo* list_files(const char* dir_path, int* n_files) {
	*n_files = 0;
	
	// open the directory 
	DIR* dir;
	struct dirent* entry;
	dir = opendir(dir_path);
	if (dir == NULL) {
		return NULL;
	}

	// prepare buffer to concat dir_path with file name in dir
	// in the form "<dir_path>/<file_name>"
	size_t dir_path_len = strlen(dir_path);
	char* file_path = malloc(dir_path_len + 1 + MAX_FILE_NAME_LEN);
	memcpy(file_path, dir_path, dir_path_len);
	file_path[dir_path_len] = '/';
	char* file_path_name_part = file_path + dir_path_len + 1;

	// go through all regular files in directory, store the name and checksum
	// in a linked list
	struct FileInfo* info_list = NULL;
	while ((entry = readdir(dir)) != NULL) {
		// create the path to this file (stored in file_path buffer)
		memcpy(file_path_name_part, entry->d_name, MAX_FILE_NAME_LEN);
		
		// if not regular file, skip this entry
		if (!is_regular_file(file_path)) {
			continue;
		}

		// create a new linked list node to store file info
		struct FileInfo* node = malloc(sizeof(struct FileInfo));
		// store name with null terminator
		memcpy(node->name, entry->d_name, MAX_FILE_NAME_LEN-1);
		node->name[MAX_FILE_NAME_LEN-1] = 0;
		// store checksum
		FILE* fd = fopen(file_path, "r");
		node->checksum = crc32_file_checksum(fd);
		fclose(fd);

		// add node to linked list
		// here, we add the node to the top of list, because it's easier
		node->next = info_list;
		info_list = node;
		(*n_files)++;
	}

	free(file_path);
	return info_list;
}


void free_file_info(struct FileInfo* file_info_list) {
	struct FileInfo* head = file_info_list;
	while (file_info_list != NULL) {
		head = file_info_list;
		file_info_list = file_info_list->next;
		free(head);
	}
}


char* join_path(const char* p1, const char* p2) {
	size_t len1 = strlen(p1);
	size_t len2 = strlen(p2);
	// 2 extra char for slash and null terminator
	char* path = malloc(len1 + len2 + 2); 
	memcpy(path, p1, len1);
	path[len1] = '/';
	// end path with username and null terminator
	memcpy(path + len1 + 1, p2, len2 + 1);
	return path;
}


char* path_to_user(const char* username) {
	return join_path(DATABASE_DIR, username);
}