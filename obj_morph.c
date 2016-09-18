#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#include <GL/glext.h>
#define glGetProcAddress(n) glXGetProcAddressARB((GLubyte *) n)

#include "obj.h"
#include "obj_morph.h"

struct morph_vert
{
    //float u[3];
    float n[3];
    float t[2];
    float v[3];
    
    float mv0[3];	// morph target 0 vertex
    float mn0[3];	// morph target 0 normal
    float mv1[3];	// morph target 1 vertex
    float mn1[3];	// morph target 1 normal
    float mv2[3];	// morph target 2 vertex
    float mn2[3];	// morph target 2 normal
    float mv3[3];	// morph target 3 vertex
    float mn3[3];	// morph target 3 normal
    float mv4[3];	// morph target 4 vertex
    float mn4[3];	// morph target 4 normal

};

struct morph_data
{
    int objfile;
    GLuint prog;
    GLuint vbo;

    int mc;					// number of material
    int mm;
    int vc;					// number of vertex
    int vm;
    int sc;					// number of surface
    int sm;

    struct obj_mtrl *mv;	// array of materials
    struct morph_vert *vv;	// array of vert
    struct obj_surf *sv;	// array of surface
};

/*---------------------------------------------------------------------------*/
/* Global morph State                                                         */

morph_data* gMorpher = NULL;
static GLboolean Morph_GL_is_initialized;

// attribute index (vertex, normal)
int vLoc[5], nLoc[5];

static void morph_init_gl(void)
{
    if (!gMorpher)
    	return;
    	
    if (Morph_GL_is_initialized == 0)
    {
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB,
                     &GL_max_texture_image_units);

        GL_has_vertex_buffer_object = gl_ext("GL_ARB_vertex_buffer_object");
        GL_has_multitexture         = gl_ext("GL_ARB_multitexture");

#ifndef __APPLE__
        glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYARBPROC)
                      glGetProcAddress("glEnableVertexAttribArrayARB");
        glVertexAttribPointer     = (PFNGLVERTEXATTRIBPOINTERARBPROC)
                      glGetProcAddress("glVertexAttribPointerARB");
        glGenBuffers              = (PFNGLGENBUFFERSARBPROC)
                      glGetProcAddress("glGenBuffersARB");
        glBindBuffer              = (PFNGLBINDBUFFERARBPROC)
                      glGetProcAddress("glBindBufferARB");
        glBufferData              = (PFNGLBUFFERDATAARBPROC)
                      glGetProcAddress("glBufferDataARB");
        glDeleteBuffers           = (PFNGLDELETEBUFFERSARBPROC)
                      glGetProcAddress("glDeleteBuffersARB");
        glActiveTexture           = (PFNGLACTIVETEXTUREARBPROC)
                      glGetProcAddress("glActiveTextureARB");
#endif
        Morph_GL_is_initialized = 1;
    }
}

void morph_init_data(void)
{
    if (!gMorpher)
    	return;
    	
    if (gMorpher->vbo == 0 && GL_has_vertex_buffer_object)
    {
        int si;
		//printf("morph_init_file...\n");
        
        /* Store all vertex data in a vertex buffer object. */

        glGenBuffers(1, &gMorpher->vbo);
        glBindBuffer(GL_ARRAY_BUFFER_ARB, gMorpher->vbo);
        glBufferData(GL_ARRAY_BUFFER_ARB,
                     gMorpher->vc * sizeof (struct morph_vert),
                     gMorpher->vv,  GL_STATIC_DRAW_ARB);

        /* Store all index data in index buffer objects. */

        for (si = 0; si < gMorpher->sc; ++si)
        {
            if (gMorpher->sv[si].pc > 0)
            {
                glGenBuffers(1, &gMorpher->sv[si].pibo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,
                             gMorpher->sv[si].pibo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB,
                             gMorpher->sv[si].pc * sizeof (struct obj_poly),
                             gMorpher->sv[si].pv, GL_STATIC_DRAW_ARB);
            }

            if (gMorpher->sv[si].lc > 0)
            {
                glGenBuffers(1, &gMorpher->sv[si].libo);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB,
                             gMorpher->sv[si].libo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER_ARB,
                             gMorpher->sv[si].lc * sizeof (struct obj_line),
                             gMorpher->sv[si].lv, GL_STATIC_DRAW_ARB);
            }
        }
    }
}

void morph_draw_vert(void)
{
    GLsizei s = sizeof (struct morph_vert);

    morph_init_gl();

    /* Enable all necessary vertex attribute pointers. */

    //glEnableVertexAttribArray(6);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableVertexAttribArray(vLoc[0]);
    glEnableVertexAttribArray(nLoc[0]);
    glEnableVertexAttribArray(vLoc[1]);
    glEnableVertexAttribArray(nLoc[1]);
    glEnableVertexAttribArray(vLoc[2]);
    glEnableVertexAttribArray(nLoc[2]);
    glEnableVertexAttribArray(vLoc[3]);
    glEnableVertexAttribArray(nLoc[3]);
    glEnableVertexAttribArray(vLoc[4]);
    glEnableVertexAttribArray(nLoc[4]);
     
    if (GL_has_vertex_buffer_object)
    {
        /* Bind attributes to a vertex buffer object. */

        glBindBuffer(GL_ARRAY_BUFFER_ARB, gMorpher->vbo);

        glNormalPointer      (      GL_FLOAT,    s, OFFSET(0));
        glTexCoordPointer    (2,    GL_FLOAT,    s, OFFSET(12));
        glVertexPointer      (3,    GL_FLOAT,    s, OFFSET(20));
        glVertexAttribPointer(vLoc[0], 3, GL_FLOAT, 0, s, OFFSET(32));
        glVertexAttribPointer(nLoc[0], 3, GL_FLOAT, 0, s, OFFSET(44));
        glVertexAttribPointer(vLoc[1], 3, GL_FLOAT, 0, s, OFFSET(56));
        glVertexAttribPointer(nLoc[1], 3, GL_FLOAT, 0, s, OFFSET(68));
        glVertexAttribPointer(vLoc[2], 3, GL_FLOAT, 0, s, OFFSET(80));
        glVertexAttribPointer(nLoc[2], 3, GL_FLOAT, 0, s, OFFSET(92));
        glVertexAttribPointer(vLoc[3], 3, GL_FLOAT, 0, s, OFFSET(104));
        glVertexAttribPointer(nLoc[3], 3, GL_FLOAT, 0, s, OFFSET(116));
        glVertexAttribPointer(vLoc[4], 3, GL_FLOAT, 0, s, OFFSET(128));
        glVertexAttribPointer(nLoc[4], 3, GL_FLOAT, 0, s, OFFSET(140));
    }
    else
    {
        /* Bind attributes in main memory. */

        glNormalPointer      (      GL_FLOAT,    s, gMorpher->vv->n);
        glTexCoordPointer    (2,    GL_FLOAT,    s, gMorpher->vv->t);
        glVertexPointer      (3,    GL_FLOAT,    s, gMorpher->vv->v);
        glVertexAttribPointer(vLoc[0], 3, GL_FLOAT, 0, s, gMorpher->vv->mv0);
        glVertexAttribPointer(nLoc[0], 3, GL_FLOAT, 0, s, gMorpher->vv->mn0);
        glVertexAttribPointer(vLoc[1], 3, GL_FLOAT, 0, s, gMorpher->vv->mv1);
        glVertexAttribPointer(nLoc[1], 3, GL_FLOAT, 0, s, gMorpher->vv->mn1);
        glVertexAttribPointer(vLoc[2], 3, GL_FLOAT, 0, s, gMorpher->vv->mv2);
        glVertexAttribPointer(nLoc[2], 3, GL_FLOAT, 0, s, gMorpher->vv->mn2);
        glVertexAttribPointer(vLoc[3], 3, GL_FLOAT, 0, s, gMorpher->vv->mv3);
        glVertexAttribPointer(nLoc[3], 3, GL_FLOAT, 0, s, gMorpher->vv->mn3);
        glVertexAttribPointer(vLoc[4], 3, GL_FLOAT, 0, s, gMorpher->vv->mv4);
        glVertexAttribPointer(nLoc[4], 3, GL_FLOAT, 0, s, gMorpher->vv->mn4);
    }
}

void morph_draw_surf(int si)
{
    struct obj_surf *sp = gMorpher->sv + si;

    morph_init_gl();

    if (0 < sp->pc || sp->lc > 0)
    {
        // James: do not apply any material for this morpher
        /* Apply this surface's material. */
        //if (0 <= sp->mi && sp->mi < gMorpher->mc)
        //    obj_draw_mtrl(gMorpher->objfile, sp->mi);

        /* Render all polygons. */

        if (sp->pibo)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, sp->pibo);
            glDrawElements(GL_TRIANGLES, 3 * sp->pc,
                           GL_UNSIGNED_INT, OFFSET(0));
        }

        /* Render all lines. */

        if (sp->libo)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, sp->libo);
            glDrawElements(GL_LINES, 2 * sp->lc,
                           GL_UNSIGNED_INT, OFFSET(0));
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, 0);
    }
}

void morph_draw_data(void)
{
    if (!gMorpher)
    	return;
    
    int si;
    morph_init_gl();
    morph_init_data();

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
    glPushAttrib(GL_LIGHTING_BIT | GL_TEXTURE_BIT | GL_ENABLE_BIT);
    {
        glDisable(GL_COLOR_MATERIAL);
        /* Load the vertex buffer. */

        morph_draw_vert();
        
        /* Render each surface. */

        for (si = 0; si < gMorpher->sc; ++si)
            morph_draw_surf(si);

    }
    glPopAttrib();
    glPopClientAttrib();
}

static void morph_read_v(const char *line)
{
    int _vi;

    /* Parse a vertex position. */

    if ((_vi = add_v()) >= 0)
    {
        sscanf(line, "%f %f %f", _vv[_vi].v + 0,
                                 _vv[_vi].v + 1,
                                 _vv[_vi].v + 2);
        _vv[_vi]._ii = -1;
    }
    
    float v[3];
    sscanf(line, "%f %f %f", v + 0, v + 1, v + 2);
}

int mprph_process_obj(int prog, int fi, char* infile)
{
	
	if (gMorpher)
		return 0;
	
	FILE *fin;
	if ((fin = fopen(infile, "r")))
    {
    
    	char buf[MAXSTR];
    	float v[3];
    	
    	fgets (buf, MAXSTR, fin);	// version string
    	fgets (buf, MAXSTR, fin);	// enpty line
    	
		gMorpher = new morph_data;
		gMorpher->objfile = fi;
		gMorpher->prog = prog;
	
		// grab all data from obj file object copy thing...
		gMorpher->vbo	= file(fi)->vbo;
		gMorpher->mc	= file(fi)->mc;
		gMorpher->mm	= file(fi)->mm;
		gMorpher->vc	= file(fi)->vc;
		gMorpher->vm	= file(fi)->vm;
		gMorpher->sc	= file(fi)->sc;
		gMorpher->sm	= file(fi)->sm;
	
		// just ref material and surf info from neutral obj_file
		gMorpher->mv = file(fi)->mv;
		gMorpher->sv = file(fi)->sv;
	
		// allocate morph vertex
		gMorpher->vv = (struct morph_vert *) malloc(gMorpher->vc * sizeof(struct morph_vert));
	
		//printf("head has %i vert, %i surf\n", gMorpher->vc, gMorpher->sc);
	
		// copy data from obj
		struct morph_vert* _vv;
		struct obj_vert *_ov;
	
		for (int i=0; i < gMorpher->vc; i++)
		{
			_vv = gMorpher->vv + i;
			_ov = file(fi)->vv + i;

			// set neutral vertex data
			_vv->n[0] = _ov->n[0];
			_vv->n[1] = _ov->n[1];
			_vv->n[2] = _ov->n[2];
		
			_vv->t[0] = _ov->t[0];
			_vv->t[1] = _ov->t[1];
		
			_vv->v[0] = _ov->v[0];
			_vv->v[1] = _ov->v[1];
			_vv->v[2] = _ov->v[2];
		
			// set morph target data
		
			// morph delta0
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mv0[0] = v[0];
			_vv->mv0[1] = v[1];
			_vv->mv0[2] = v[2];
	
			// morph normal0
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mn0[0] = v[0];
			_vv->mn0[1] = v[1];
			_vv->mn0[2] = v[2];

			// morph delta1
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mv1[0] = v[0];
			_vv->mv1[1] = v[1];
			_vv->mv1[2] = v[2];
	
			// morph normal1
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mn1[0] = v[0];
			_vv->mn1[1] = v[1];
			_vv->mn1[2] = v[2];

			// morph delta2
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mv2[0] = v[0];
			_vv->mv2[1] = v[1];
			_vv->mv2[2] = v[2];
	
			// morph normal2
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mn2[0] = v[0];
			_vv->mn2[1] = v[1];
			_vv->mn2[2] = v[2];

			// morph delta3
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mv3[0] = v[0];
			_vv->mv3[1] = v[1];
			_vv->mv3[2] = v[2];
	
			// morph normal3
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mn3[0] = v[0];
			_vv->mn3[1] = v[1];
			_vv->mn3[2] = v[2];

			// morph delta4
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mv4[0] = v[0];
			_vv->mv4[1] = v[1];
			_vv->mv4[2] = v[2];
	
			// morph normal4
			fgets (buf, MAXSTR, fin);
			sscanf(buf, "%f %f %f", v + 0, v + 1, v + 2); 
			_vv->mn4[0] = v[0];
			_vv->mn4[1] = v[1];
			_vv->mn4[2] = v[2];
	
		}
	
	}	// end of file read
	else
		return -1;
	
	
	//
	//printf("morph program id: %i\n", prog);
	glUseProgram(prog);
	
	vLoc[0] = glGetAttribLocation(prog, "coordMorph0");
	vLoc[1] = glGetAttribLocation(prog, "coordMorph1");
	vLoc[2] = glGetAttribLocation(prog, "coordMorph2");
	vLoc[3] = glGetAttribLocation(prog, "coordMorph3");
	vLoc[4] = glGetAttribLocation(prog, "coordMorph4");
	
	nLoc[0] = glGetAttribLocation(prog, "normalMorph0");
	nLoc[1] = glGetAttribLocation(prog, "normalMorph1");
	nLoc[2] = glGetAttribLocation(prog, "normalMorph2");
	nLoc[3] = glGetAttribLocation(prog, "normalMorph3");
	nLoc[4] = glGetAttribLocation(prog, "normalMorph4");
	
	glUseProgram(0);
	
	return 1;
	
}

void morph_clean_up(void)
{
	if (!gMorpher)
		return;
	
	// only take care of what we create here
	// referenced obj_file data will be cleaned separately
	
	// remove vbo
	if (gMorpher->vbo) glDeleteBuffers(1, &gMorpher->vbo);
	gMorpher->vbo = 0;
	
	// free memory...
	gMorpher->mv = NULL;
	gMorpher->sv = NULL;
	free(gMorpher->vv);
	delete gMorpher;
	gMorpher = NULL;
	
	//printf("morph data cleaned...\n");
}

int morph_write_data(int fi, int f0, int f1, int f2, int f3, int f4, char* out)
{
	FILE *fout;
	
	if ((fout = fopen(out, "w")))
	{
		fprintf(fout, "morph data (5 target) ver. 0.1\n\n");
	
		struct obj_vert *_ov, *_v0, *_v1, *_v2, *_v3, *_v4;
		float value0, value1, value2;
	
		for (int i=0; i < file(fi)->vc; i++)
		{
			// obj_vert pointer
			_ov = file(fi)->vv + i;		// neutral
			_v0 = file(f0)->vv + i;		// target 0
			_v1 = file(f1)->vv + i;		// target 1
			_v2 = file(f2)->vv + i;		// target 2
			_v3 = file(f3)->vv + i;		// target 3
			_v4 = file(f4)->vv + i;		// target 4
				
			// morph delta0
			value0 = _v0->v[0] - _ov->v[0];
			value1 = _v0->v[1] - _ov->v[1];
			value2 = _v0->v[2] - _ov->v[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
		
			// morph normal0
			value0 = _v0->n[0] - _ov->n[0];
			value1 = _v0->n[1] - _ov->n[1];
			value2 = _v0->n[2] - _ov->n[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);

			// morph delta1
			value0 = _v1->v[0] - _ov->v[0];
			value1 = _v1->v[1] - _ov->v[1];
			value2 = _v1->v[2] - _ov->v[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
	
			// morph normal1
			value0 = _v1->n[0] - _ov->n[0];
			value1 = _v1->n[1] - _ov->n[1];
			value2 = _v1->n[2] - _ov->n[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);

			// morph delta2
			value0 = _v2->v[0] - _ov->v[0];
			value1 = _v2->v[1] - _ov->v[1];
			value2 = _v2->v[2] - _ov->v[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
	
			// morph normal2
			value0 = _v2->n[0] - _ov->n[0];
			value1 = _v2->n[1] - _ov->n[1];
			value2 = _v2->n[2] - _ov->n[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);

			// morph delta3
			value0 = _v3->v[0] - _ov->v[0];
			value1 = _v3->v[1] - _ov->v[1];
			value2 = _v3->v[2] - _ov->v[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
	
			// morph normal3
			value0 = _v3->n[0] - _ov->n[0];
			value1 = _v3->n[1] - _ov->n[1];
			value2 = _v3->n[2] - _ov->n[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);

			// morph delta4
			value0 = _v4->v[0] - _ov->v[0];
			value1 = _v4->v[1] - _ov->v[1];
			value2 = _v4->v[2] - _ov->v[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
	
			// morph normal4
			value0 = _v4->n[0] - _ov->n[0];
			value1 = _v4->n[1] - _ov->n[1];
			value2 = _v4->n[2] - _ov->n[2];
			fprintf(fout, "%12.8f %12.8f %12.8f\n", value0, value1, value2);
	
		}
	
		fclose(fout);
		
	}
	else
		return -1;

	return 1;
}
