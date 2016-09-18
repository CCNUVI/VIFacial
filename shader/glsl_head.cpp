/* Facial Animation
 * author : xiecong
 * email  : xiconxi@gmail.com
 * Sep. 18, 2016
 *
 *************************************************************/

// complier commad in linux 
// g++ *.cpp -lGL -lGLEW -lglut -fpermissive -lGLU -lpng -ljpeg 
#include <GL/glew.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h> 
#include <math.h>

#include "obj.c"
#include "obj_morph.c"

// framerate display
#include "framerate.h"

// obj model id
int theModel, morph0, morph1, morph2, morph3, morph4;

// screen related
int windowWidth = 512;
int windowHeight = 512;
float scRatio = 1.0f;

// mouse track
float mouseWindowX = 0.0;
float mouseWindowY = 0.0;

// temp storage for image data	
char femaleData[1600000];	// 1024*1024
char maleData[1600000];

// shader program id
GLuint pDefault;
GLuint pMorph;
GLuint pTile;
GLuint pAll;

// shader modes
#define SHADERDEFAULT	1
#define SHADERMORPH		2
#define	SHADERALL		3
#define SHADERTILE		4
int mode = SHADERDEFAULT;

// tile shader uniform var
float numTiles = 32.0f;
float threshTile = 0.15f;
int tileMode = 0.0f;

float lpos[4] = {1.0, 0.5, 1.0, 0.0};

// texture map id
GLuint femaleTex, maleTex;


// offscreen buffer
GLuint fb, color_tex, depth_rb;

// useful clamp macro
#define LIMIT(x,min,max) { if ((x)>(max)) (x)=(max); if ((x)<(min)) (x)=(min);}

////////////////////////////////////////////////////////////////////////////////
static void CHECK_FRAMEBUFFER_STATUS(void) 
{ 
	printf("Checking framebuffer status.\n");
	GLenum status; 
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT); 
	switch(status) { 
		case GL_FRAMEBUFFER_COMPLETE_EXT: 
		printf("Framebuffer complete.\n");
		break; 
		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
		printf("Framebuffer unsupported.\n");
		/* choose different formats */ 
		break; 
		default: 
		/* programming error; will fail on all hardware */ 
		assert(0); 
	}
}

void addRenderTexture(int width, int height)
{
	
	// rtt texture
	glGenTextures(1, &color_tex);
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_INT, NULL);
	
	// frame buffer
	glGenFramebuffersEXT(1, &fb);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, color_tex, 0);
	
	// depth renderbuffer
	glGenRenderbuffersEXT(1, &depth_rb);
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_rb);
	
	// check status
	CHECK_FRAMEBUFFER_STATUS();
		
	// make the window to target
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		
	glBindTexture(GL_TEXTURE_2D, 0);
	
}

////////////////////////////////////////////////////////////////////////////////
void resizeRenderTexture(int width, int height)
{
	// initialize color texture
	glBindTexture(GL_TEXTURE_2D, color_tex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
                     GL_RGBA, GL_INT, NULL);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
                                  GL_COLOR_ATTACHMENT0_EXT,
                                  GL_TEXTURE_2D, color_tex, 0);

	// initialize depth renderbuffer
	glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depth_rb);
	glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT,
                                 GL_DEPTH_COMPONENT24, width, height);
	glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT,
                                     GL_DEPTH_ATTACHMENT_EXT,
                                     GL_RENDERBUFFER_EXT, depth_rb);

}

////////////////////////////////////////////////////////////////////////////////
char *textFileRead(char *fn)
{
	FILE *fp;
	char *content = NULL;

	int count=0;

	if (fn != NULL) {
		fp = fopen(fn,"rt");

		if (fp != NULL) {
      
      fseek(fp, 0, SEEK_END);
      count = ftell(fp);
      rewind(fp);

			if (count > 0) {
				content = (char *)malloc(sizeof(char) * (count+1));
				count = fread(content,sizeof(char),count,fp);
				content[count] = '\0';
			}
			fclose(fp);
		}
	}
	
	if (content == NULL)
	   {
	   fprintf(stderr, "ERROR: could not load in file %s\n", fn);
	   exit(1);
	   }
	return content;
}           

////////////////////////////////////////////////////////////////////////////////
void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).
	if(h == 0)
		h = 1;

	scRatio = 1.0 * w / h;

	// Reset the coordinate system before modifying
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	// Set the viewport to be the entire window
    glViewport(0, 0, w, h);
    
    windowWidth = w;
    windowHeight = h;

	// Set the correct perspective.
	gluPerspective(45, scRatio, 1, 1000);
	glMatrixMode(GL_MODELVIEW);
	
	// RTT stuff
	resizeRenderTexture(w, h);

}

////////////////////////////////////////////////////////////////////////////////
void printShaderLog(GLuint prog)
{
    GLint infoLogLength = 0;
    GLsizei charsWritten  = 0;
    GLchar *infoLog;

    glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        infoLog = (char *) malloc(infoLogLength);
        glGetShaderInfoLog(prog, infoLogLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
        free(infoLog);
    }
}

////////////////////////////////////////////////////////////////////////////////
void printProgramLog(GLuint shad)
{
    GLint infoLogLength = 0;
    GLsizei charsWritten  = 0;
    GLchar *infoLog;

    glGetProgramiv(shad, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        infoLog = (char *) malloc(infoLogLength);
        glGetProgramInfoLog(shad, infoLogLength, &charsWritten, infoLog);
		printf("%s\n",infoLog);
        free(infoLog);
    }
}

////////////////////////////////////////////////////////////////////////////////
float blinkTimer = 0.0f;
float morphWeight0 = 0.0f; float morphDelta0 = 0.0f;
float morphWeight1 = 0.0f; float morphDelta1 = 0.0f;
float morphWeight2 = 0.0f; float morphDelta2 = 0.0f;
float morphWeight3 = 0.0f; float morphDelta3 = 0.0f;
float morphWeight4 = 0.0f; float morphDelta4 = 0.0f;
float unWrapWeight = 0.0f; float unWrapDelta = 0.0f;
float useColorCode = 0.0f;

void setMorphParams(GLuint p)
{
								
	// blink : random blink
	if (morphDelta0 == 0.0f)
	{
		blinkTimer -= gElapsedTime;
		if (blinkTimer < 0)
		{
			// start blinking
			morphDelta0 = 3.0f / gFPS;
			
			// set next timer
			blinkTimer = 3.0f + (rand() % 10) / 5.0f;
		}
	}
	else
	{
		morphWeight0 += morphDelta0;
		if (morphWeight0 > 1.0f)
		{
			morphWeight0 = 1.0f;
			morphDelta0 = -fabs(morphDelta0);
		}
		if (morphWeight0 < 0.0f)
		{
			morphWeight0 = 0.0f;
			morphDelta0 = 0.0f;
		}
	}
		
	// smile
	morphWeight1 += morphDelta1;
	if (morphWeight1 > 1.0f)
	{
		morphWeight1 = 1.0f;
		morphDelta1 = -fabs(morphDelta1);
	}
	if (morphWeight1 < 0.0f)
	{
		morphWeight1 = 0.0f;
		morphDelta1 = 0.0f;
	}
		
	// fear
	morphWeight2 += morphDelta2;
	if (morphWeight2 > 1.0f)
	{
		morphWeight2 = 1.0f;
		morphDelta2 = -fabs(morphDelta2);
	}
	if (morphWeight2 < 0.0f)
	{
		morphWeight2 = 0.0f;
		morphDelta2 = 0.0f;
	}

	// aah
	morphWeight3 += morphDelta3;
	if (morphWeight3 > 1.0f)
	{
		morphWeight3 = 1.0f;
		morphDelta3 = -fabs(morphDelta3);
	}
	if (morphWeight3 < 0.0f)
	{
		morphWeight3 = 0.0f;
		morphDelta3 = 0.0f;
	}

	// male
	morphWeight4 += morphDelta4;
	if (morphWeight4 > 1.0f)
	{
		morphWeight4 = 1.0f;
		morphDelta4 = 0.0f;
	}
	if (morphWeight4 < 0.0f)
	{
		morphWeight4 = 0.0f;
		morphDelta4 = 0.0f;
	}

	
	// set uniformation location
	glUniform1f(glGetUniformLocation(p,"morphWeight0"), morphWeight0);
	glUniform1f(glGetUniformLocation(p,"morphWeight1"), morphWeight1);
	glUniform1f(glGetUniformLocation(p,"morphWeight2"), morphWeight2);
	glUniform1f(glGetUniformLocation(p,"morphWeight3"), morphWeight3);
	glUniform1f(glGetUniformLocation(p,"morphWeight4"), morphWeight4);
	glUniform1f(glGetUniformLocation(p,"useColorCode"), useColorCode);

}

////////////////////////////////////////////////////////////////////////////////
float modelDepth0 = -14.870f;	// male -14.601
float modelDepth1 = 9.403f;		// male 10.201
int unWrapMode = 0;

void setAllParams(GLuint p)
{
								
	// blink : random blink
	if (morphDelta0 == 0.0f)
	{
		blinkTimer -= gElapsedTime;
		if (blinkTimer < 0)
		{
			// start blinking
			morphDelta0 = 3.0f / gFPS;
			
			// set next timer
			blinkTimer = 3.0f + (rand() % 10) / 5.0f;
		}
	}
	else
	{
		morphWeight0 += morphDelta0;
		if (morphWeight0 > 1.0f)
		{
			morphWeight0 = 1.0f;
			morphDelta0 = -fabs(morphDelta0);
		}
		if (morphWeight0 < 0.0f)
		{
			morphWeight0 = 0.0f;
			morphDelta0 = 0.0f;
		}
	}
		
	// smile
	morphWeight1 += morphDelta1;
	if (morphWeight1 > 1.0f)
	{
		morphWeight1 = 1.0f;
		morphDelta1 = -fabs(morphDelta1);
	}
	if (morphWeight1 < 0.0f)
	{
		morphWeight1 = 0.0f;
		morphDelta1 = 0.0f;
	}
		
	// fear
	morphWeight2 += morphDelta2;
	if (morphWeight2 > 1.0f)
	{
		morphWeight2 = 1.0f;
		morphDelta2 = -fabs(morphDelta2);
	}
	if (morphWeight2 < 0.0f)
	{
		morphWeight2 = 0.0f;
		morphDelta2 = 0.0f;
	}

	// aah
	morphWeight3 += morphDelta3;
	if (morphWeight3 > 1.0f)
	{
		morphWeight3 = 1.0f;
		morphDelta3 = -fabs(morphDelta3);
	}
	if (morphWeight3 < 0.0f)
	{
		morphWeight3 = 0.0f;
		morphDelta3 = 0.0f;
	}

	// male
	morphWeight4 += morphDelta4;
	if (morphWeight4 > 1.0f)
	{
		morphWeight4 = 1.0f;
		morphDelta4 = 0.0f;
	}
	if (morphWeight4 < 0.0f)
	{
		morphWeight4 = 0.0f;
		morphDelta4 = 0.0f;
	}

	// unwrap
	unWrapWeight += unWrapDelta;
	if (unWrapWeight > 1.0f)
	{
		unWrapWeight = 1.0f;
		unWrapDelta = 0.0f;
	}
	if (unWrapWeight < 0.0f)
	{
		unWrapWeight = 0.0f;
		unWrapDelta = 0.0f;
	}
	
	// set uniformation location
	glUniform1f(glGetUniformLocation(p,"morphWeight0"), morphWeight0);
	glUniform1f(glGetUniformLocation(p,"morphWeight1"), morphWeight1);
	glUniform1f(glGetUniformLocation(p,"morphWeight2"), morphWeight2);
	glUniform1f(glGetUniformLocation(p,"morphWeight3"), morphWeight3);
	glUniform1f(glGetUniformLocation(p,"morphWeight4"), morphWeight4);
	glUniform1f(glGetUniformLocation(p,"unWrapWeight"), unWrapWeight);
	
	glUniform1f(glGetUniformLocation(p,"modelDepth0"), modelDepth0);
	glUniform1f(glGetUniformLocation(p,"modelDepth1"), modelDepth1);
	glUniform1i(glGetUniformLocation(p,"unWrapMode"), unWrapMode);

}

////////////////////////////////////////////////////////////////////////////////
void setTileParams(GLuint p)
{
	// set uniformation location
	glUniform1f(glGetUniformLocation(p,"NumTiles"),numTiles);
	glUniform1f(glGetUniformLocation(p,"Threshhold"),threshTile);
	glUniform1f(glGetUniformLocation(p,"scRatio"),scRatio);
	glUniform1i(glGetUniformLocation(p,"tileMode"),tileMode);
}

////////////////////////////////////////////////////////////////////////////////
void drawHead(void)
{
    // draw obj file
    glPushMatrix();
    glScalef(0.15, 0.15, 0.15);
    glRotatef(-90,0,1,0);
    
    obj_draw_file(theModel);
    
    glPopMatrix();
}

////////////////////////////////////////////////////////////////////////////////
void drawRTT(void)
{
	// First we bind the FBO so we can render to it
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);

	// Then render as normal
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	
	// normal texture set
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, femaleTex);	
	
	// draw real stuff
	glUseProgram(pTile);
	glUniform1i(glGetUniformLocation(pTile, "rttMap"), 0);
	glUniform1i(glGetUniformLocation(pTile, "obj"), 0);
	drawHead();
	
	// Restore default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	
	// clear the default frame buffer we are going to render to
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// reset perspective to screen space (orthogonal projection)  
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,1,0,1,-1,1);

	// Now bind the first texture to use it
	glEnable(GL_TEXTURE_2D);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, color_tex);
		
	if (mode == SHADERTILE)
	{
		setTileParams(pTile);
		glUseProgram(pTile);
		glUniform1i(glGetUniformLocation(pTile, "rttMap"), 0);
		glUniform1i(glGetUniformLocation(pTile, "obj"), 1);
	}
	glPushAttrib(GL_TEXTURE_BIT);
	glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f(0, 0);
		glTexCoord2f(1, 0);
		glVertex2f(1, 0);
		glTexCoord2f(1, 1);
		glVertex2f(1, 1);
		glTexCoord2f(0, 1);
		glVertex2f(0, 1);
	glEnd();
	glPopAttrib();

	// restore projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45, scRatio, 1, 1000);
	glMatrixMode(GL_MODELVIEW);

    glBindTexture(GL_TEXTURE_2D, 0);

}


////////////////////////////////////////////////////////////////////////////////
#define DIST_F 1024
void renderScene(void) {
		
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update framerate
	framerateUpdate();
	
	// camera setup
	glLoadIdentity();
	gluLookAt(0.0,-0.2,6.0, 
		      0.0,-0.2,-1.0,
			  0.0f,1.0f,0.0f);

	glLightfv(GL_LIGHT0, GL_POSITION, lpos);
	glRotatef(mouseWindowX*90+90,0,1,0);
	glRotatef(mouseWindowY*30,0,0,1);
    
    // default shader: static textured head   
	if (mode == SHADERDEFAULT)
	{
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, femaleTex);
			
		glUseProgram(pDefault);
		glUniform1i(glGetUniformLocation(pDefault, "colorTexture"), 0);
		drawHead();
	}
	
	// morph shader without texture
	if (mode == SHADERMORPH)
	{
		glUseProgram(pMorph);
		setMorphParams(pMorph);
		glPushMatrix();
    	glScalef(0.15, 0.15, 0.15);
    	glRotatef(-90,0,1,0);
		morph_draw_data();
		glPopMatrix();
	
	}
	
	// morph shader with texture
	if (mode == SHADERALL)
	{
		glEnable(GL_BLEND);
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GREATER,0);
		
		glUseProgram(pAll);
		setAllParams(pAll);
		
		glEnable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, femaleTex);
		glUniform1i(glGetUniformLocation(pAll, "femaleTexture"), 0);
		
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, maleTex);
		glUniform1i(glGetUniformLocation(pAll, "maleTexture"), 1);

		glPushMatrix();
    	glScalef(0.15, 0.15, 0.15);
    	glRotatef(-90,0,1,0);
		morph_draw_data();
		glPopMatrix();
	}

	// tile shader: post processing (compositor)
	if (mode == SHADERTILE)
		drawRTT();
	
	// done. let's show it
	glutSwapBuffers();
}

////////////////////////////////////////////////////////////////////////////////
void processNormalKeys(unsigned char key, int x, int y) {

	if (key == 27) 
	{
		morph_clean_up();
		obj_del_file(theModel);
		exit(0);
	}	
    else if (key == '1')
    	mode = SHADERDEFAULT;
    else if (key == '2')
    {
        if (mode != SHADERMORPH)
        {
        	mode = SHADERMORPH;
        	
        	// reset values
			blinkTimer = 0.0f;
			morphWeight0 = morphDelta0 = 0.0f;
			morphWeight1 = morphDelta1 = 0.0f;
			morphWeight2 = morphDelta2 = 0.0f;
			morphWeight3 = morphDelta3 = 0.0f;
			morphWeight4 = morphDelta4 = 0.0f;     
			useColorCode = 0.0f;   	
        }
    }
	else if (key == '3')			// all feature combined except RTT
	{
        if (mode != SHADERALL)
        {
        	mode = SHADERALL;
        	
        	// reset values
			blinkTimer = 0.0f;
			morphWeight0 = morphDelta0 = 0.0f;
			morphWeight1 = morphDelta1 = 0.0f;
			morphWeight2 = morphDelta2 = 0.0f;
			morphWeight3 = morphDelta3 = 0.0f;
			morphWeight4 = morphDelta4 = 0.0f;
			unWrapWeight = 0.0f;  	
        }	
	}
    else if (key == '4')			// composition: tile
    {
    	if (mode != SHADERTILE)
    	{
    		numTiles = 64.0f;
			threshTile = 0.15f;
			tileMode = 0.0f;
    	}
    	mode = SHADERTILE;
    }
	else if (key == 'r')			// reset tile configuration
	{
		numTiles = 64.0f;
		threshTile = 0.15f;
	}
	
	// morph key
	if (mode == SHADERMORPH || mode == SHADERALL )
	{
		if (key == 's')			// smile
		{
			if (morphDelta1 == 0.0f)
			{
				morphDelta1 = 1.5f / gFPS;
			}
		}
		else if (key == 'f')	// fear
		{
			if (morphDelta2 == 0.0f)
			{
				morphDelta2 = 1.5f / gFPS;
			}
		}
		else if (key == 'a')	// big aah
		{
			if (morphDelta3 == 0.0f)
			{
				morphDelta3 = 2.0f / gFPS;
			}
		}
		else if (key == 'c')	// male / female
		{
			if (morphDelta4 == 0.0f)
			{
				if (morphWeight4 == 0.0f)
				{
					morphDelta4 = 0.7f / gFPS;
					modelDepth0 = -14.601f;
					modelDepth1 = 10.201f;
				}
				else
				{
					morphDelta4 = -0.7f / gFPS;
					modelDepth0 = -14.870f;
					modelDepth1 = 9.403f;
				}
			}
		}
		else if (key == 'd')
		{
			useColorCode = fabs(fabs(useColorCode) - 1.0f);
		}
		else if (key == 'u')	// unwrap (linear)
		{
			if (unWrapDelta == 0.0f)
			{
				unWrapMode = 0;
				if (unWrapWeight == 0.0f)
					unWrapDelta = 0.25f / gFPS;
				else
					unWrapDelta = -0.25f / gFPS;
			}
		}
		else if (key == 'i')	// unwrap (sin)
		{
			if (unWrapDelta == 0.0f)
			{
				unWrapMode = 1;
				if (unWrapWeight == 0.0f)
					unWrapDelta = 0.25f / gFPS;
				else
					unWrapDelta = -0.25f / gFPS;
			}
		}
		else if (key == 'o')	// unwrap (?)
		{
			if (unWrapDelta == 0.0f)
			{
				unWrapMode = 2;
				if (unWrapWeight == 0.0f)
					unWrapDelta = 0.25f / gFPS;
				else
					unWrapDelta = -0.25f / gFPS;
			}
		}
		else if (key == 'p')	// unwrap (?)
		{
			if (unWrapDelta == 0.0f)
			{
				unWrapMode = 3;
				if (unWrapWeight == 0.0f)
					unWrapDelta = 0.25f / gFPS;
				else
					unWrapDelta = -0.25f / gFPS;
			}
		}
		
	}
	else if (mode == SHADERTILE)
	{
		// can change comositor effect mode here
		if (key == 't')			// common tile
		{
			tileMode = 0;
		}
		else if (key == 'y')	// mirrored tile
		{
			tileMode = 1;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
void processSpecialKeys(int key, int x, int y)
{
	if (mode == SHADERTILE)
	{
		if (key == GLUT_KEY_RIGHT) 
			numTiles += 1.0f;
		else if (key == GLUT_KEY_LEFT)
			numTiles -= 1.0f;
		else if (key == GLUT_KEY_UP)
			threshTile += 0.01f;
		else if (key = GLUT_KEY_DOWN)
			threshTile -= 0.01f;
		
		LIMIT(numTiles, 8, windowHeight / 4.0f);
		LIMIT(threshTile, 0.0f, 0.3f);	
	}
}

////////////////////////////////////////////////////////////////////////////////
GLuint setShaders(char * vert, char * frag) 
{
    GLuint v,f, pro;
	char *vs,*fs;

	v = glCreateShader(GL_VERTEX_SHADER);
	f = glCreateShader(GL_FRAGMENT_SHADER);

	vs = textFileRead(vert);
	fs = textFileRead(frag);

	const char * vv = vs;
	const char * ff = fs;

	glShaderSource(v, 1, &vv,NULL);
	glShaderSource(f, 1, &ff,NULL);

	free(vs);free(fs);

	glCompileShader(v);
	glCompileShader(f);

	printShaderLog(v);
	printShaderLog(f);

	pro = glCreateProgram();
	glAttachShader(pro,v);
	glAttachShader(pro,f);

	glLinkProgram(pro);
	printProgramLog(pro);
	
	return(pro);
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
int setupTexture(char * texData, int w, int h)
{
    GLuint o = 0;
	glGenTextures(1, &o);
    glBindTexture(GL_TEXTURE_2D, o);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB,
        GL_UNSIGNED_BYTE, (const GLvoid *) texData);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	return o;
}

////////////////////////////////////////////////////////////////////////////////
void doTexStuff(char * dataPath, char * fileName, int width, int height,
		char * TheArray)
{
	register int x, y;
	char fullPath[256];
	int arrayLoc;
	FILE * imageFile;
	int maxVal =0;
	float ratio;

	strcpy(fullPath, dataPath);
	strcat(fullPath, fileName);
	imageFile = fopen(fullPath, "r");
	
	if (imageFile == NULL) 
		fprintf(stderr,"Cannot find texture file in data directory: %s\n", fullPath);
	else
	{	    
		for (y = 0; y < height*width*3; y+=1) /// rgb texture this time ...
		{
			TheArray[y]	= (char) fgetc(imageFile);
		}            

		fclose(imageFile);
	}
}

////////////////////////////////////////////////////////////////////////////////
// upper left 0,0
// negative to left or above the window
void PassiveMouseMotion(int x, int y)
{
	if (x<0)
		mouseWindowX = -1;
	else if (x>windowWidth)
		mouseWindowX = 1;
	else
		mouseWindowX = 2* (x / (float) windowWidth) - 1.0;
       
	if (y<0)
		mouseWindowY = -1;
	else if (y>windowHeight)
		mouseWindowY = 1;
	else
		mouseWindowY = 2* (y / (float) windowHeight) - 1.0;
}

////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv) 
{

	// GL initialization
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(windowWidth, windowHeight);
	glutCreateWindow("Facial Animation");

	// set window title again for framerate display
	framerateTitle("Facial Animation");

	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	glutReshapeFunc(changeSize);
	glutKeyboardFunc(processNormalKeys);
	glutSpecialFunc(processSpecialKeys);
	glutPassiveMotionFunc(PassiveMouseMotion);

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	
	glEnable(GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glewInit();
	if (GLEW_ARB_vertex_shader && GLEW_ARB_fragment_shader)
		printf("Ready for GLSL\n");
	else {
		printf("No GLSL support\n");
		exit(1);
	}
	
	// load in the obj model
	theModel = obj_add_file("data/maya_obj_1/neutral_uv.obj");
	
	// female skin texture		
	doTexStuff("./data/maya_obj_1/", "female_skin.raw",  1024, 512, femaleData);
	femaleTex = setupTexture(femaleData, 1024, 512);
	
	// male skin texture
	doTexStuff("./data/maya_obj_1/", "male_skin.raw",  1024, 512, maleData);
	maleTex = setupTexture(maleData, 1024, 512);

	// add frame buffer object(RTT texture)
	addRenderTexture(windowWidth, windowHeight);
		
	// setup shaders
	pDefault	= setShaders("./shader/default.vert", "./shader/textured.frag");
	pMorph 		= setShaders("./shader/morph.vert", "./shader/morph.frag");
	pTile 		= setShaders("./shader/default.vert", "./shader/tile.frag"); 
	pAll		= setShaders("./shader/merged.vert", "./shader/merged.frag");
	
	// process morph data
	mprph_process_obj(pMorph, theModel, "data/morph_1.dat");
		
	// start main loop
	glutMainLoop();
	
	// clean up mem stuff
	morph_clean_up();
	obj_del_file(theModel);
	
	glDeleteFramebuffersEXT(1, &fb);
	glDeleteRenderbuffersEXT(1, &depth_rb);

	
	return 0;
}

