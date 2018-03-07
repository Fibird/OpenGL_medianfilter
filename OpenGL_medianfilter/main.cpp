#include <iostream>
#include <GL/glew.h>
#include <glut.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Reader.h"

using namespace cv;
using namespace std;

// FBO identifier
GLuint fb;
// Texture identifiers
GLuint wTexID;
GLuint rTexID;

struct structTextureParameters
{
	GLenum texTarget;
	GLenum texInternalFormat;
	GLenum texFormat;
	char *shader_source;
}textureParameters;

// GLSL vars
GLuint glslProgram;
GLuint fragmentShader;

void initFBO(unsigned unWidth, unsigned unHeight);
void setupTexture(const GLuint texID, unsigned width, unsigned height);
void createTextures(unsigned width, unsigned height, unsigned char *data);
void initGLSL(void);
void performComputation(unsigned width, unsigned height);
void transferFromTexture(unsigned char* data, unsigned width, unsigned height);
void transferToTexture(unsigned char* data, GLuint texID, unsigned width, unsigned height);

int main(int argc, char **argv)
{
	// Read an noised image
	if (argc != 2)
	{
		cout << "No arguments found!" << endl;
		return -1;
	}
	Mat img = imread(argv[1], CV_LOAD_IMAGE_GRAYSCALE);
	Mat rst = Mat::zeros(img.size(), CV_8UC1);

	if (!img.data)
	{
		cout << "Could not open or find the image!" << endl;
		return -1;
	}

	// Create variables for GL
	textureParameters.texTarget = GL_TEXTURE_RECTANGLE_ARB;		// Type of texture
	textureParameters.texInternalFormat = GL_RGB32F_ARB;		// Internal format of texture
	textureParameters.texFormat = GL_LUMINANCE;					// Format of texture, single channel

	// Set up GLUT 
	glutInit(&argc, argv);
	GLuint glutWindowHandle = glutCreateWindow("OpenGL Convolution");
	glewInit();

	// Init framebuffer
	initFBO(img.size().width, img.size().height);
	// Create textures for vectors
	createTextures(img.size().width, img.size().height, img.data);
	// Clean the texture buffer (for security reasons)
	CReader reader;
	textureParameters.shader_source = reader.textFileRead("clean.frag");
	initGLSL();
	performComputation(img.size().width, img.size().height);
	textureParameters.shader_source = reader.textFileRead("convolution.frag");
	initGLSL();
	performComputation(img.size().width, img.size().height);
	
	// Get GPU results
	transferFromTexture(rst.data, rst.size().width, rst.size().height);

	imwrite("result.png", rst);
	// clean up
	glDetachShader(glslProgram, fragmentShader);
	glDeleteShader(fragmentShader);
	glDeleteProgram(glslProgram);
	glDeleteFramebuffersEXT(1, &fb);
	glDeleteTextures(1, &wTexID);
	glDeleteTextures(1, &rTexID);
	glutDestroyWindow(glutWindowHandle);
	return 0;
}

void initFBO(unsigned unWidth, unsigned unHeight)
{

	// Create FBO (off-screen framebuffer)
	glGenFramebuffersEXT(1, &fb);
	// Bind offscreen framebuffer (that is, skip the window-specific render target)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	// Viewport for 1:1 pixel=texture mapping
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// Set Orthogonality projection
	gluOrtho2D(0.0, unWidth, 0.0, unHeight);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, unWidth, unHeight);
}

void setupTexture(const GLuint texID, unsigned width, unsigned height)
{
	// make active and bind
	glBindTexture(textureParameters.texTarget, texID);
	// turn off filtering and wrap modes
	glTexParameteri(textureParameters.texTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(textureParameters.texTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(textureParameters.texTarget, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(textureParameters.texTarget, GL_TEXTURE_WRAP_T, GL_CLAMP);
	// define texture with floating point format
	glTexImage2D(textureParameters.texTarget, 0, textureParameters.texInternalFormat,
		width, height, 0, textureParameters.texFormat, GL_FLOAT, 0);
}

void createTextures(unsigned width, unsigned height, unsigned char *data)
{
	// Create textures
	// w is write-only; r is just read-only
	glGenTextures(1, &wTexID);
	glGenTextures(1, &rTexID);
	// Set up textures
	setupTexture(wTexID, width, height);
	setupTexture(rTexID, width, height);
	// Transfers data to texture
	transferToTexture(data, rTexID, width, height);
	// set texenv mode
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/**
* Set up the GLSL runtime and creates shader.
*/
void initGLSL(void) 
{
	// create program object
	glslProgram = glCreateProgram();
	// create shader object (fragment shader)
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER_ARB);
	// set source for shader
	const GLchar* source = textureParameters.shader_source;
	glShaderSource(fragmentShader, 1, &source, NULL);
	// compile shader
	glCompileShader(fragmentShader);

	// attach shader to program
	glAttachShader(glslProgram, fragmentShader);
	// link into full program, use fixed function vertex shader.
	// you can also link a pass-through vertex shader.
	glLinkProgram(glslProgram);
}

void performComputation(unsigned width, unsigned height) 
{
	// attach output texture to FBO
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureParameters.texTarget, wTexID, 0);

	// enable GLSL program
	glUseProgram(glslProgram);
	// enable the read-only texture x
	glActiveTexture(GL_TEXTURE0);

	// Synchronize for the timing reason.
	glFinish();

	// set render destination
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

	// Hit all texels in quad.
	glPolygonMode(GL_FRONT, GL_FILL);

	// Draw a rectangle
	// render quad with unnormalized texcoords
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, 0.0);
	glVertex2f(0.0, 0.0);
	glTexCoord2f(width, 0.0);
	glVertex2f(width, 0.0);
	glTexCoord2f(width, height);
	glVertex2f(width, height);
	glTexCoord2f(0.0, height);
	glVertex2f(0.0, height);
	glEnd();
	glFinish();
}

/**
* Transfers data to texture. Notice the difference between ATI and NVIDIA.
*/
void transferToTexture(unsigned char* data, GLuint texID, unsigned width, unsigned height) 
{
	// version (a): HW-accelerated on NVIDIA
	glBindTexture(textureParameters.texTarget, texID);
	glTexSubImage2D(textureParameters.texTarget, 0, 0, 0, width, height, textureParameters.texFormat, GL_FLOAT, data);
	// version (b): HW-accelerated on ATI
	//	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureParameters.texTarget, texID, 0);
	//	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	//	glRasterPos2i(0,0);
	//	glDrawPixels(unWidth,unHeight,textureParameters.texFormat,GL_FLOAT,data);
	//	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, textureParameters.texTarget, 0, 0);
}

/**
* Transfers data from currently texture to host memory.
*/
void transferFromTexture(unsigned char* data, unsigned width, unsigned height) 
{
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadPixels(0, 0, width, height, textureParameters.texFormat, GL_FLOAT, data);
}