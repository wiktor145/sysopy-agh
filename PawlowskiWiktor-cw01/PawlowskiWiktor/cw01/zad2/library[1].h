//
// Created by wicia on 07.03.19.
//

#ifndef SYSOPY_LIBRARY_H
#define SYSOPY_LIBRARY_H


typedef struct Array {
    char** array;    // structure which holds array of blocks and its size
    int size;
} Array;

struct Array* create_table (int size);

void set_directory_to_search(char * directory);

void set_file_to_find(char * file);

void set_tmp_file(char * tmpfile);

void remove_block(struct Array * array, int index);

void find_file_and_copy_to_temp(void);

int reserve_block (struct Array * array);

void free_all(struct Array * array);

#endif //SYSOPY_LIBRARY_H
