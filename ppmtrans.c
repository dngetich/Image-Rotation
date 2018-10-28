/* ppmtrans.c
 *
 * By James Garijo-Garde and David Ngetich
 * Completed 10/8/2018
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cputiming.h"
#include "assert.h"
#include "a2methods.h"
#include "a2plain.h"
#include "a2blocked.h"
#include "pnm.h"
#include "mem.h"

#define A2 A2Methods_UArray2

#define SET_METHODS(METHODS, MAP, WHAT) do {                    \
        methods = (METHODS);                                    \
        assert(methods != NULL);                                \
        map = methods->MAP;                                     \
        if (map == NULL) {                                      \
                fprintf(stderr, "%s does not support "          \
                                WHAT "mapping\n",               \
                                argv[0]);                       \
                exit(1);                                        \
        }                                                       \
} while (0)

static void
usage(const char *progname)
{
        fprintf(stderr, "Usage: %s [-rotate <angle>] "
                        "[-{row,col,block}-major] [filename]\n",
                        progname);
        exit(1);
}

struct closure {
        A2 array;
        A2Methods_T method_suite;
};

/* function declarations */

/* processes the image */
void process(Pnm_ppm old, A2Methods_T methods, int rotation,
             A2Methods_mapfun *map);

/* rotates the image 90 degrees */
void rotate_90(int col, int row, A2 array2, A2Methods_Object *ptr, void *cl);

/* rotates the image 180 degrees */
void rotate_180(Pnm_ppm old, Pnm_ppm new1, Pnm_ppm new2, A2Methods_T methods,
                struct closure *items, A2Methods_mapfun *map);

/* rotates the image 270 degrees */
void rotate_270(Pnm_ppm old, Pnm_ppm new1, Pnm_ppm new2, Pnm_ppm new3,
                A2Methods_T methods, struct closure *items,
                A2Methods_mapfun *map);

int main(int argc, char *argv[]) 
{

        char *time_file_name = NULL;
        int   rotation       = 0;
        int   i;
        char *filename = NULL;
        FILE *fp = NULL;

        /* the timer decartion */
        CPUTime_T timer;
        timer = CPUTime_New();
       

        /* default to UArray2 methods */
        A2Methods_T methods = uarray2_methods_plain; 
        assert(methods);


        /* default to best map */
        A2Methods_mapfun *map = methods->map_default; 
        assert(map);

        for (i = 1; i < argc; i++) {
                if (strcmp(argv[i], "-row-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_row_major, 
                                    "row-major");
                } else if (strcmp(argv[i], "-col-major") == 0) {
                        SET_METHODS(uarray2_methods_plain, map_col_major, 
                                    "column-major");
                } else if (strcmp(argv[i], "-block-major") == 0) {
                        SET_METHODS(uarray2_methods_blocked, map_block_major,
                                    "block-major");
                } else if (strcmp(argv[i], "-rotate") == 0) {
                        if (!(i + 1 < argc)) {      /* no rotate value */
                                usage(argv[0]);
                        }
                        char *endptr;
                        rotation = strtol(argv[++i], &endptr, 10);
                        if (!(rotation == 0 || rotation == 90 ||
                            rotation == 180 || rotation == 270)) {
                                fprintf(stderr, 
                                        "Rotation must be 0, 90 180 or 270\n");
                                usage(argv[0]);
                        }
                        if (!(*endptr == '\0')) {    /* Not a number */
                                usage(argv[0]);
                        }
                } else if (strcmp(argv[i], "-time") == 0) {
                        time_file_name = argv[++i];
                } else if (*argv[i] == '-') {
                        fprintf(stderr, "%s: unknown option '%s'\n", argv[0],
                                argv[i]);
                        exit(1);
                } 
                /* file name assignment here */
                else if (argc - i == 1) { /*this is bad */
                        filename = argv[i];
                } else if (argc - i > 1) {
                        fprintf(stderr, "Too many arguments\n");
                        usage(argv[0]);
                } else {
                        break;
                }
        }
        
        if (filename == NULL) {
                /* Reads from standard input */
                fp = stdin;
        } else {
                fp = fopen(filename, "rb");
        }
        assert(fp != NULL);

        /*declare a ppm_pnm file*/
        Pnm_ppm old = Pnm_ppmread(fp, methods);
        assert(old != NULL);


        if(time_file_name != NULL)
        {
                double time_used, pixel_time;
                FILE *ftime;
                ftime = fopen(time_file_name, "a");
                assert(ftime);
                int pic_h = old->height;
                int pic_w = old->width;
                int pixel_num = pic_h * pic_w;


                CPUTime_Start(timer);

                process(old, methods, rotation, map);

                time_used = CPUTime_Stop(timer);
                pixel_time = time_used / pixel_num;

                if (fp == stdin) {
                        fprintf(ftime, "Standard Input\n");

                } else {
                        fprintf(ftime, "%s\n", filename);
                }
                fprintf(ftime, "Image Height:%d\n", pic_h);
                fprintf(ftime, "Image width:%d\n", pic_w);
                fprintf(ftime, "The rotation of image takes: %.0f\n",
                    time_used);
                fprintf(ftime, "The time per pixel: %.0f\n",pixel_time);

                fclose(ftime);
        } else {
                process(old, methods, rotation, map);

        }

        fclose(fp);
        CPUTime_Free(&timer);
        exit(EXIT_SUCCESS);
}

/* processes the image old by facilitation the correct rotation and time options
 */
void process(Pnm_ppm old, A2Methods_T methods, int rotation,
             A2Methods_mapfun *map)
{
        struct closure *items = malloc(sizeof(struct closure));
        assert(items != NULL);

        items->method_suite = methods;


        /* call the mapping function with the correct apply function */
        if (rotation == 0) {
                Pnm_ppmwrite(stdout, old);
        } else if (rotation == 90) {
                Pnm_ppm new = NEW(new);

                new->width = old->height;
                new->height = old->width;
                new->denominator = old->denominator;
                int size = methods->size(old->pixels);
                new->pixels = methods->new(new->width, new->height, size);
                new->methods = old->methods;

                items->array = new->pixels;

                map(old->pixels, rotate_90, items);

                Pnm_ppmwrite(stdout, new);

                Pnm_ppmfree(&new);
        } else if (rotation == 180) {
                Pnm_ppm new1 = NEW(new1);
                Pnm_ppm new2 = NEW(new2);

                rotate_180(old, new1, new2, methods, items, map);

                Pnm_ppmwrite(stdout, new2);

                Pnm_ppmfree(&new1);
                Pnm_ppmfree(&new2);
        } else if (rotation == 270) {
                Pnm_ppm new1 = NEW(new1);
                Pnm_ppm new2 = NEW(new2);
                Pnm_ppm new3 = NEW(new3);

                rotate_270(old, new1, new2, new3, methods, items, map);

                Pnm_ppmwrite(stdout, new3);

                Pnm_ppmfree(&new1);
                Pnm_ppmfree(&new2);
                Pnm_ppmfree(&new3);
        }

        free(items);
        Pnm_ppmfree(&old);
}

/* rotates the image 90 degrees by mapping through map function map */
void rotate_90(int col, int row, A2 array2, A2Methods_Object *ptr, void *cl)
{
        A2Methods_T methods = ((struct closure *)cl)->method_suite;
        A2 new_array = ((struct closure *)cl)->array;

        int new_c = methods->height(array2)-row-1;
        int new_r = col;

        Pnm_rgb old = ptr;
        Pnm_rgb new = methods->at(new_array, new_c, new_r);

        *new = *old;

        (void)new;
}

/* rotates the image 180 degrees by following the methodology of a 90 degree
 * rotation twice */
void rotate_180(Pnm_ppm old, Pnm_ppm new1, Pnm_ppm new2, A2Methods_T methods,
                struct closure *items, A2Methods_mapfun *map)
{
        /* Rotates by 90 degrees twice */

        /* first rotation */
        new1->width = old->height;
        new1->height = old->width;
        new1->denominator = old->denominator;
        int size = methods->size(old->pixels);
        new1->pixels = methods->new(new1->width, new1->height, size);
        new1->methods = old->methods;

        items->array = new1->pixels;

        map(old->pixels, rotate_90, items);

        /* second rotation */

        new2->width = new1->height;
        new2->height = new1->width;
        new2->denominator = new1->denominator;
        size = methods->size(new1->pixels);
        new2->pixels = methods->new(new2->width, new2->height, size);
        new2->methods = new1->methods;

        items->array = new2->pixels;

        map(new1->pixels, rotate_90, items);
}

/* rotates the image 270 degrees by following the methodology of a 90 degree
 * rotation and then a 180 degree rotation */
void rotate_270(Pnm_ppm old, Pnm_ppm new1, Pnm_ppm new2, Pnm_ppm new3,
                A2Methods_T methods, struct closure *items,
                A2Methods_mapfun *map)
{
        /* Rotates by 90 degrees and then by 180 degrees */

        /* 90 degree rotation */
        new1->width = old->height;
        new1->height = old->width;
        new1->denominator = old->denominator;
        int size = methods->size(old->pixels);
        new1->pixels = methods->new(new1->width, new1->height, size);
        new1->methods = old->methods;

        items->array = new1->pixels;

        map(old->pixels, rotate_90, items);

        /* 180 degree rotation */
        rotate_180(new1, new2, new3, methods, items, map);
}