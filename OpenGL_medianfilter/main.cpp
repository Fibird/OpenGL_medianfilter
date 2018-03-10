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
typedef float data_type;

struct structTextureParameters
{
	GLenum texTarget;
	GLenum texInternalFormat;
	GLenum texFormat;
	char *shader_source;
}textureParameters;

// GLSL vars
//GLuint glslProgram;
//GLuint fragmentShader;

void initFBO(unsigned unWidth, unsigned unHeight);
void setupTexture(const GLuint texID, unsigned width, unsigned height);
void createTextures(unsigned width, unsigned height);
//bool initGLSL(void);
void runShader(unsigned width, unsigned height, GLuint glslProgram);
void transferFromTexture(void * data, GLenum type, unsigned width, unsigned height);
void transferToTexture(void * data, GLenum type, GLuint texID, unsigned width, unsigned height);
bool buildShader(const GLchar *source, GLuint *shader, GLuint *program);
void cleanShader(GLuint &shader, GLuint &program);

int main(int argc, char **argv)
{
	// Read an noised image
	/*if (argc != 2)
	{
		cout << "No arguments found!" << endl;
		return -1;
	}
	Mat img = imread(argv[1], CV_LOAD_IMAGE_COLOR);
	Mat rst = Mat::zeros(img.size(), CV_8UC4);*/

	/*if (!img.data)
	{
		cout << "Could not open or find the image!" << endl;
		return -1;
	}*/
	unsigned int width, height;
	width = 10; height = 10;
	data_type *img = new data_type[width * height * 4];
	GLenum type = GL_FLOAT;
	for (unsigned int i = 0; i < width * height * 4; ++i)
		img[i] = 1;
	// Create variables for GL
	textureParameters.texTarget = GL_TEXTURE_RECTANGLE_ARB;		// Type of texture
	textureParameters.texInternalFormat = GL_RGBA32F_ARB;		// Internal format of texture
	textureParameters.texFormat = GL_RGBA;					// Format of texture, single channel

	// Set up GLUT 
	glutInit(&argc, argv);
	GLuint glutWindowHandle = glutCreateWindow("OpenGL Convolution");
	glewInit();

	// Init framebuffer
	initFBO(width, height);
	// Create textures for vectors
	createTextures(width, height);
	//createTextures(wTexID, type, width, height);
	// Transfers data to texture
	transferToTexture(img, type, rTexID, width, height);
	// Clean the texture buffer (for security reasons)
	CReader reader;
	//textureParameters.shader_source = reader.textFileRead("clean.frag");
	char *shader_source = reader.textFileRead("clean.frag");
	
	GLuint fragShader, program;
	bool buildSuccess = buildShader(shader_source, &fragShader, &program);
	if (!buildSuccess)
	{
		return -1;
	}
	runShader(width, height, program);

	shader_source = reader.textFileRead("convolution.frag");
	buildSuccess = buildShader(shader_source, &fragShader, &program);
	if (!buildSuccess)
	{
		return -1;
	}
	runShader(width, height, program);
	data_type *rst = new data_type[width * height * 4];
	// Get GPU results
	transferFromTexture(rst, type, width, height);

	for (unsigned int i = 0; i < width * height * 4; i++)
		cout << rst[i] << endl;
	//imwrite("result.png", rst);
	// clean up
	cleanShader(fragShader, program);
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

void createTextures(unsigned width, unsigned height)
{
	// Create textures
	// w is write-only; r is just read-only
	glGenTextures(1, &wTexID);
	glGenTextures(1, &rTexID);
	// Set up textures
	setupTexture(wTexID, width, height);
	setupTexture(rTexID, width, height);
}


bool buildShader(const GLchar *source, GLuint *shader, GLuint *program)
{
	GLint compiled, linked;
	// create program object
	*program = glCreateProgram();
	// create shader object (fragment shader)
	*shader = glCreateShader(GL_FRAGMENT_SHADER_ARB);
	// set source for shader
	//const GLchar* source = textureParameters.shader_source;
	glShaderSource(*shader, 1, &source, NULL);
	
	// compile shader
	glCompileShader(*shader);
	// Get compiling result
	glGetShaderiv(*shader, GL_COMPILE_STATUS, &compiled);
	if (!compiled)
	{
		GLint length;
		GLchar *log;
		glGetShaderiv(*shader, GL_INFO_LOG_LENGTH, &length);
		log = new GLchar[length];
		glGetShaderInfoLog(*shader, length, &length, log);
		cerr << "=====Shader Compliation Error=====\n" << log << endl;
		delete log;
		return false;
	}
	// attach shader to program
	glAttachShader(*program, *shader);
	glLinkProgram(*program);
	
	glGetProgramiv(*program, GL_LINK_STATUS, &linked);
	if (!linked)
	{
		GLint length;
		GLchar *log;
		glGetProgramiv(*program, GL_INFO_LOG_LENGTH, &length);
		log = new GLchar[length];
		glGetProgramInfoLog(*program, length, &length, log);
		cerr << "=====Shader Link Error=====\n" << log << endl;
		delete log;
		return false;
	}
	return true;
}

void runShader(unsigned width, unsigned height, GLuint glslProgram) 
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

void cleanShader(GLuint &shader, GLuint &program)
{
	glDetachShader(program, shader);
	glDeleteShader(shader);
	glDeleteProgram(program);
}

/**
* Transfers data to texture. Notice the difference between ATI and NVIDIA.
*/
void transferToTexture(void* data, GLenum type, GLuint texID, unsigned width, unsigned height)
{
	// version (a): HW-accelerated on NVIDIA
	glBindTexture(textureParameters.texTarget, texID);
	glTexSubImage2D(textureParameters.texTarget, 0, 0, 0, width, height, textureParameters.texFormat, type, data);
	// set texenv mode
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
}

/**
* Transfers data from currently texture to host memory.
*/
void transferFromTexture(void* data, GLenum type, unsigned width, unsigned height)
{
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glReadPixels(0, 0, width, height, textureParameters.texFormat, type, data);
}

/**
* Sets up a floating point texture with the NEAREST filtering.
*/
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
	glTexImage2D(textureParameters.texTarget, 0, textureParameters.texInternalFormat, width, height, 0, textureParameters.texFormat, GL_FLOAT, 0);
}
