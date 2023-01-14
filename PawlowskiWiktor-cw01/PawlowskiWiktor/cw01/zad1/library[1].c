//
// Created by wicia on 07.03.19.
//

#include "library.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // necessary for strlen

char* temp_file = NULL;
char* current_dir = NULL;
char* current_file = NULL;

struct Array* create_table (int size)   //creating Array for blocks
{
    if ( size <= 0 )
    {
        printf("Can't create array with size less than or equal to 0. exiting\n");
        free_all(NULL);
        exit(1);
    }

    struct Array* array = calloc(1, sizeof(struct Array));          // calloc (numberofelements, sizeofelement)

    if (array == NULL)
    {
        printf("Error in creating Array. exiting\n");
        free_all(NULL);
        exit(1);
    }

    array->array= (char**) calloc(size, sizeof(char*));
    if (array->array == NULL)
    {
        printf("Error in creating array. exiting\n");
        free (array);
        free_all(NULL);
        exit(1);
    }
    for (int i=0; i<size; i++) array->array[i]=NULL;

    array->size = size;

    return array;
}

void set_directory_to_search(char * directory)  //setting current directory
{
    if (directory == NULL) return;

    if (current_dir != NULL) free(current_dir);

    current_dir = (char*) calloc(strlen(directory)+1, sizeof(char));

    if (current_dir == NULL)
    {
        printf("Error, exiting\n");
        free_all(NULL);
        exit(1);
    }

    strcpy(current_dir,directory);


}

void set_file_to_find(char * file)  //setting file to find
{
    if (file == NULL) return;

    if (current_file != NULL) free(current_file);

    current_file= (char*) calloc(strlen(file)+1, sizeof(char));
    if (current_file == NULL)
    {
        printf("Error, exiting\n");
        free_all(NULL);
        exit(1);
    }

    strcpy(current_file,file);

}

void set_tmp_file(char * tmpfile)
{
    if (tmpfile == NULL) return;

    if (temp_file != NULL) free(temp_file);

    temp_file = calloc (strlen(tmpfile)+1, sizeof(char));
    if (temp_file == NULL)
    {
        printf("Error, exiting\n");
        free_all(NULL);
        exit(1);
    }

    strcpy(temp_file,tmpfile);

}

void remove_block(struct Array * array, int index)
{
    if ( array == NULL || array->array == NULL)
    {
        printf("Array is NULL\n");
        return;
    }

    if (index < 0 || index >= array->size )
    {
        printf("Index is out of bounds\n");
        return;
    }

    if (array->array[index] != NULL) free(array->array[index]);
    array->array[index]=NULL;

}

void find_file_and_copy_to_temp(void)
{
    if (temp_file == NULL)
    {
        printf("No valid temp file!\n");
        free_all(NULL);
        exit(1);
    }

    char command[1000];  // buffer which holds command to execute

    snprintf(command, 1000, "find %s -name %s -type f &> %s",current_dir,current_file,temp_file);
    // function which concatenates command into one char*

    if (system(command) == -1)  //finding file
    {
        free_all(NULL);

        printf("Error in finding file\n");
        exit(1);
    }
}

int reserve_block (struct Array * array)
{
    if (array == NULL || array->array == NULL)
    {
        printf("Array is NULL!\n");
        free_all(NULL);

        exit(1);
    }

    if (temp_file == NULL)
    {
        printf("No tmp file, returning\n");
        free_all(NULL);

        exit(1);
    }

    int index;  //index of block in which space will be allocated

    for (index = 0; index < array->size && array->array[index] != NULL; index++);  //finding free space in array

    if (index == array->size)  //no free memory in array
    {
        printf("No available memory in array! Try creating bigger array\n");
        free_all(NULL);

        exit(1);
    }

    FILE * f = fopen(temp_file, "r");  //opening temporary file

    if (f == NULL)
    {
        printf("Error in opening temporary file!\n");
        free_all(NULL);

        exit(1);
    }

    long filesize;
    if (fseek(f, 0L, SEEK_END) != 0)   // using fseek to get the last pointer position in file to get file size
    {
        printf("Error in reading file\n");
        rewind(f);
        fclose(f);
        free_all(NULL);

        exit(1);
    }

    filesize = ftell(f); //getting position of pointer == file size
    rewind(f);  //rewinding pointer to the begining of file

    if (filesize == 0)
    {
        printf("Temp file is empty. Probably searching gave no result. Leaving NULL in array\n");
        return index;
    }

    array->array[index]= (char*) calloc(1, filesize+1);  //calloc a memory for file

    if (array->array[index] == NULL)
    {
        printf("Error in allocating memory\n");
        fclose(f);
        free_all(NULL);

        exit(1);
    }

    if (1 != fread(array->array[index], filesize, 1, f))  //getting entire file to array;
        // fread (to, sizeofelement, no_of_elements,fromwhere))
    {
        printf("Error in reading file\n");
        fclose(f);
        free(array->array[index]);
        array->array[index]=NULL;
        free_all(NULL);

        exit(1);
    }

    fclose(f);

    if (array->array[index] != NULL)
    {
        array->array[index][filesize] = '\0';   // terminating char* with nullbyte
        return index;
    }
    else
    {
        printf ("Error, exiting\n");
        free_all(NULL);
        exit(1);
    }

}

void free_all(struct Array * array)
{// this function frees char** array and temp file, current directory and current file
    if (temp_file != NULL) free(temp_file);
    if (current_file != NULL) free(current_file);
    if (current_dir != NULL) free(current_dir);


    if (array == NULL || array->array == NULL) return;

    for (int i=0; i < array->size; i++)
        if (array->array[i]!=NULL) free(array->array[i]);

    free(array->array);

}





