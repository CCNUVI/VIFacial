#ifndef OBJMORPH_H
#define OBJMORPH_H

/*===========================================================================*/

static void morph_init_gl(void);
void morph_init_data(void);
int mprph_process_obj(int prog, int fi, char* infile);
int morph_write_data(int fi, int f0, int f1, int f2, int f3, int f4, char* out);

void morph_draw_vert(void);
void morph_draw_surf(int si);
void morph_draw_data(void);

void morph_clean_up(void);

/*===========================================================================*/

#endif
