/*
 * fat_routines.h
 *
 *  Created on: 14 Oct 2026
 *      Author: marku
 */

#ifndef INC_FAT_ROUTINES_H_
#define INC_FAT_ROUTINES_H_

#include <stdbool.h>
#include <stdint.h>

void print_dir(char *path);	// DOS style directory listing of the flash disk, path may be NULL
void type_file(char *filename);	// DOS style type: show the contents of a file
void remove_file(char *filename);	// DOS style del

bool file_exists(char *filename);
bool file_add(char *filename, char *txt);	// add text at the end, makes the file when not there
bool file_first_line(char *filename, char *line, uint16_t size);


#endif /* INC_FAT_ROUTINES_H_ */
