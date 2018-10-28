/* James Garijo-Garde and David Ngetich
 *
 * HW 3 uarray2b.c
 *
 * Completed on 10/8/2018
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <uarray.h>
#include "uarray2.h"
#include "uarray2b.h"

#define T UArray2b_T

struct T {
        int width;
        int height;
        int size;
        int blocksize;
        UArray2_T the_array; 
} ;

struct closurep {
        T array;
        void (*outer_apply)(int col, int row, T array2b, void *elem, void *cl);
        void *original_cl;
} ;

/*apply function for that intiatializes the elements into the UArray2b */
void block_maker(int row, int col, UArray2_T uarray2, void *curr_value,
                 void *cl);

/*apply function for the Uarray2b_free that frees every block at a time */ 
void block_free(int row, int col, UArray2_T uarray2, void *curr_value,
                 void *cl);

/* new blocked 2d array
*  blocksize = square root of # of cells in block.
*  blocksize < 1 is a checked runtime error  */
T UArray2b_new (int w, int h, int s, int bsize)
{
        T array = malloc(sizeof(*array));
        assert(array);
        array->width = w;
        array->height = h;
        array->size = s;
        array->blocksize = bsize;
        array->the_array = UArray2_new(ceil((double)w/(double)bsize),
            ceil((double)h/(double)bsize), bsize * bsize * s);
        int *closure = &s;
        UArray2_map_row_major(array->the_array, block_maker, closure );
        return array;
}

/* new blocked 2d array: blocksize as large as possible provided
*  block occupies at most 64KB (if possible)  */
T UArray2b_new_64K_block(int w, int h, int s)
{
        int bsize = floor(sqrt(65536 / s));

        return (UArray2b_new(w, h, s, bsize));

}

/* function: block_maker
 * parametreS; takes all the parameters of an apply function
 * returns: void
 * Does: it initializes every block of the UArray2b */
void block_maker(int row, int col, UArray2_T uarray2, void *curr_value,
                 void *cl)
{
        (void)row;
        (void)col;

        int size = *(int *)cl;
        int length = UArray2_size(uarray2) / size;

        *(UArray_T *) curr_value = UArray_new(length, size);
        (void)curr_value;
}

/*returns the width of the UArray2b */
int UArray2b_width(T array2b)
{
        assert(array2b != NULL);
        return array2b->width;
}

/* returns the height of the UArray2b */
int UArray2b_height(T array2b)
{
        assert(array2b != NULL);
        return array2b->height;
}

/* returns the size of the UArray2b */
int UArray2b_size(T array2b)
{
        assert(array2b != NULL);
        return array2b->size;
}

/*returns the blocksize of each block in UArray2b */
int UArray2b_blocksize(T array2b)
{
        assert(array2b != NULL);
        return array2b->blocksize;
}

/* Function: Uarray2b_free
 *Paramters: T *array2b
 * Returns: void 
 * Does: frees all the alloocated memory in the UArray2b */
void UArray2b_free(T *array2b)
{
        assert(array2b != NULL);

        UArray2_map_row_major((*array2b)->the_array, block_free, NULL);

        UArray2_free(&((*array2b)->the_array));
        free(*array2b);
}

/* Function: at
 * Parameters: T array2b, int col, int row
 * returns the void pointer to the value at the passed in index */
void *UArray2b_at(T array2b, int col, int row)
{
        assert(array2b != NULL);
        int bsize = array2b->blocksize;

        int index = (bsize * (col % bsize)) + (row % bsize);

        UArray_T temp = *(UArray_T *)UArray2_at(array2b->the_array, col/bsize,
                          row / bsize);


        return UArray_at(temp, index);
}

/* Function: block_free
 * Parameters: takes in all the parameters relevant to an apply function
 * returns:void
 * Does:frees all the memory in the block which is the UArray_T */
void block_free(int row, int col, UArray2_T uarray2, void *curr_value,
                 void *cl)
{
        (void)row;
        (void)col;
        (void)uarray2;
        (void)cl;

        UArray_free((UArray_T *)curr_value);
}


/* Function: map_block_major
 * visits every cell in one block before moving to another block 
 * It ignores the empty elements in the block and should not visit them*/

void UArray2b_map(T array2b, void apply(int col, int row, T array2b,
 void *elem, void *cl), void *cl)
{
        assert(array2b);
        int bsize = array2b->blocksize;

        /*iterate through the entire row */
        for(int block_row = 0; block_row < array2b->height; block_row++)
        {
                /*iterate through the columns */
                for(int block_col = 0; block_col < array2b->width; block_col++)
                {
                     for (int inner_row = 0; inner_row < bsize; inner_row++) {
                        for (int inner_col = 0; inner_col < bsize; inner_col++)
                        {
                                int r_index = block_row * bsize + inner_row;
                                int c_index = block_col * bsize + inner_col;
                                
                                /*validating thebounds of the block major */
                                if (r_index < array2b->height &&
                                    c_index < array2b->width) {
                                       void *at = UArray2b_at(array2b, c_index,
                                            r_index);
                                       apply(c_index, r_index, array2b, at,
                                              cl);
                                }
                        }
                     }

                }
        }

}
