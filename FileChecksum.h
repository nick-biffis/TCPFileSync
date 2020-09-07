/**
 * Contains functions to compute the checksum of a file
 */

#ifndef FILE_CHECKSUM_H_
#define FILE_CHECKSUM_H_


#include <stdio.h>   /* file IO */
#include <stdint.h>  /* integer types of exact size */


/**
 * Compute the checksum of the given file
 *
 * @param fd The file descriptor for the file
 * @return   The CRC-32 checksum of the file
 */
uint_fast32_t crc32_file_checksum(FILE *fd);


#endif // FILE_CHECKSUM_H_
