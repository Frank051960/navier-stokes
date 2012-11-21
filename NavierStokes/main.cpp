#if defined(_APPLE_)
  #include <OpenGL/OpenGL.h>
#elif defined(_WIN32)
  #include <windows.h>
  
  #define GLEW_STATIC
  #include <GL/glew.h> 
  #include <GL/gl.h> 
  #include <GL/glu.h> 
#else
  #include <GL/glew.h>
#endif

#define _USE_MATH_DEFINES 
#include <cmath>

#include <FreeImage.h>

#include <GL/glfw.h>
#include <fstream>
#include <iostream>
#include <sstream>

#include <glm/glm.hpp> //vec3, vec4, ivec4, mat4
#include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective
#include <glm/gtc/type_ptr.hpp> //value_ptr

#include "solver.h"

static int WINDOW_WIDTH = 1280;
static int WINDOW_HEIGHT = 800;

static bool drawingDensity = true;

static int NW;
static int NH;
static float dt, diff, visc;
static float force, source;

static float *u, *v, *u_prev, *v_prev;
static float *dens, *dens_prev;

static float *color_r, *color_r_prev;
static float *color_r_u, *color_r_v, *color_r_u_prev, *color_r_v_prev;

static float *color_g, *color_g_prev;
static float *color_g_u, *color_g_v, *color_g_u_prev, *color_g_v_prev;

static float *color_b, *color_b_prev;
static float *color_b_u, *color_b_v, *color_b_u_prev, *color_b_v_prev;

enum ColorMode {
  RED,
  GREEN,
  BLUE,
  COLOR_MODE_MAX
};

static unsigned int colorMode = RED;

static float forward, right, up = 0.0f;
static float yRotation = 0.0f;

int omx, omy;

void addDensity(float* density, int i, int j, float source)
{
	density[IX(i - 1, j)] = source;

	density[IX(i - 1, j - 1)] = source;
	density[IX(i, j - 1)] = source;
	density[IX(i + 1, j - 1)] = source;

	density[IX(i + 1, j)] = source;

	density[IX(i + 1, j + 1)] = source;
	density[IX(i, j + 1)] = source;
	density[IX(i - 1, j + 1)] = source;

	density[IX(i, j)] = source; 
}

void input() {
  
  // keys
  
  float speed = -1.0f;
  
  if (glfwGetKey('W') == GLFW_PRESS) {
    forward += -speed * dt;
  }
  
  if (glfwGetKey('S') == GLFW_PRESS) {
    forward -= -speed * dt;
  }
  
  if (glfwGetKey('A') == GLFW_PRESS) {
    right -= speed * dt;
  }
  
  if (glfwGetKey('D') == GLFW_PRESS) {
    right += speed * dt;
  }
  
  if (glfwGetKey('E') == GLFW_PRESS) {
    up += speed * dt;
  }
  
  if (glfwGetKey('Q') == GLFW_PRESS) {
    up -= speed * dt;
  }

  if (glfwGetKey(GLFW_KEY_LEFT)) {
    yRotation -= speed * dt * dt;
  }
  
  if (glfwGetKey(GLFW_KEY_RIGHT)) {
    yRotation += speed * dt * dt;
  }

	unsigned int size = (NW+2)*(NH+2);

	memset(dens_prev, 0, sizeof(float) * size);
	memset(v_prev, 0, sizeof(float) * size);
	memset(u_prev, 0, sizeof(float) * size);

	memset(color_r_prev, 0, sizeof(float) * size);
	memset(color_r_u_prev, 0, sizeof(float) * size);
	memset(color_r_v_prev, 0, sizeof(float) * size);

	memset(color_b_prev, 0, sizeof(float) * size);
	memset(color_b_u_prev, 0, sizeof(float) * size);
	memset(color_b_v_prev, 0, sizeof(float) * size);

	memset(color_g_prev, 0, sizeof(float) * size);
	memset(color_g_u_prev, 0, sizeof(float) * size);
	memset(color_g_v_prev, 0, sizeof(float) * size);
  
  // mouse
	
  int mx, my; 
  glfwGetMousePos(&mx, &my);
  int i = (int)((mx /(float)WINDOW_WIDTH)*NW+1);
  int j = (int)(((WINDOW_HEIGHT-my) /(float)WINDOW_HEIGHT)*NH+1);

  if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) || glfwGetKey('V')) {
    u[IX(i,j)] = force * (mx-omx);
    v[IX(i,j)] = force * (omy-my);

    color_r_u[IX(i,j)] = force * (mx-omx);
    color_r_v[IX(i,j)] = force * (omy-my);

    color_g_u[IX(i,j)] = force * (mx-omx);
    color_g_v[IX(i,j)] = force * (omy-my);

    color_b_u[IX(i,j)] = force * (mx-omx);
    color_b_v[IX(i,j)] = force * (omy-my);
  }
  

  if (glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT) || glfwGetKey('D')) {

		addDensity(dens_prev, i, j, source);

    if (colorMode == RED) {
			addDensity(color_r_prev, i, j, source);     
    }

    if (colorMode == GREEN) {
      addDensity(color_g_prev, i, j, source);     
    }

    if (colorMode == BLUE) {
      addDensity(color_b_prev, i, j, source);     
    }
  }
    
  omx = mx;
	omy = my;
}

void update() {
  stepVelocity(NW, NH, color_r_u, color_r_v, color_r_u_prev, color_r_v_prev, visc, dt);
  stepDensity(NW, NH, color_r, color_r_prev, color_r_u, color_r_v, diff, dt);

  stepVelocity(NW, NH, color_g_u, color_g_v, color_g_u_prev, color_g_v_prev, visc, dt);
  stepDensity(NW, NH, color_g, color_g_prev, color_g_u, color_g_v, diff, dt);

  stepVelocity(NW, NH, color_b_u, color_b_v, color_b_u_prev, color_b_v_prev, visc, dt);
  stepDensity(NW, NH, color_b, color_b_prev, color_b_u, color_b_v, diff, dt);
}

void drawVelocity() {
  float hw = 1.0f / NW;
	float hh = 1.0f / NH;
  
	glColor3f(1.0f, 1.0f, 1.0f);
	glLineWidth(1.0f);
  
	glBegin (GL_LINES);
  
  for (int i = 1; i <= NW ; i++) {
    
    float x = (i-0.5f) * hw - 0.5;
    
    for (int j=1; j <= NH ; j++) {
 
      float y = (j-0.5f)* hh - 0.5;
      
      glVertex2f(x, y);
      glVertex2f(x+u[IX(i,j)], y+v[IX(i,j)]);
    }
  }
  
	glEnd ();
}

void drawDensityComponent(int index, float x, float y) 
{	
	float d = dens[index];
	float cr = color_r[index];
	float cg = color_g[index];
	float cb = color_b[index];
	glColor4f(cr, cg, cb, 1.0f); 
	glVertex2f(x, y);
}


void drawDensity() {

  float hw = 1.0f/NW;
	float hh = 1.0f/NH;
  
	glBegin (GL_QUADS);
  
  for (int i = 0 ; i <= NW ; i++) {
    float x = (i - 0.5f) * hw - 0.5;
    
    for (int j = 0 ; j <= NH ; j++) {
      float y = (j - 0.5f) * hh - 0.5;
			drawDensityComponent(IX(i, j), x, y);
			drawDensityComponent(IX(i + 1, j), x + hw, y);
			drawDensityComponent(IX(i + 1, j + 1), x + hw , y + hh);
			drawDensityComponent(IX(i, j + 1), x, y + hh);
    }
  }
	glEnd ();
}

void render() {
  glTranslatef(0, 0, forward);
  
  if (drawingDensity) drawDensity();
  else drawVelocity();
}

std::string file2string(const std::string& filePath) {
  std::ifstream fileStream(filePath.c_str());
  std::stringstream textStream;
  textStream << fileStream.rdbuf();
  return textStream.str();
}

void printLog(GLuint obj) {
	int infologLength = 0;
	const int maxLength = 256;

	char infoLog[maxLength];
  
	if (glIsShader(obj)) {
		glGetShaderInfoLog(obj, maxLength, &infologLength, infoLog);
  }
	else {
		glGetProgramInfoLog(obj, maxLength, &infologLength, infoLog);
  }
  
	if (infologLength > 0) {
		printf("%s\n",infoLog);
  }
}

GLuint createShaderProgram() {
  std::string vertexFile = file2string("navierstokes.v.glsl");
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  const char *vertexSource = vertexFile.c_str();
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);
  printLog(vertexShader);
  
  std::string fragmentFile = file2string("navierstokes.f.glsl");
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  const char *fragmentSource = fragmentFile.c_str();
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);
  printLog(fragmentShader);
  
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glLinkProgram(shaderProgram);
  
  printLog(shaderProgram);
  
  return shaderProgram;
}

void keyCallback(int keyCode, int action) {
  if (keyCode == 'V' && action == GLFW_PRESS) {
    drawingDensity = !drawingDensity;
  }

  if (keyCode == 'B' && action == GLFW_PRESS) {
    colorMode++;
    if (colorMode == COLOR_MODE_MAX) {
      colorMode = 0;
    }
  }

  if (keyCode == 'C' && action == GLFW_PRESS) {
    drawingDensity = !drawingDensity;
  }
}

BYTE* LoadTexture(const char* filename, unsigned int* width, unsigned int* height, unsigned int *bpp) {
	FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);
	
	if(fif == FIF_UNKNOWN) { 
		fif = FreeImage_GetFIFFromFilename(filename);
	}
	
	if(fif == FIF_UNKNOWN) {
		return 0;
	}

	
	FIBITMAP *dib = 0;
	if(FreeImage_FIFSupportsReading(fif)) {
		dib = FreeImage_Load(fif, filename);
	}

	if(!dib) {
		return 0;
	}

	//dib = FreeImage_ConvertTo24Bits(dib);

	unsigned int pitch = FreeImage_GetPitch(dib);

	*bpp = FreeImage_GetBPP(dib);

	FIBITMAP* scaled = FreeImage_Rescale(dib, NW+2, NH+2, FILTER_BILINEAR);

	BYTE* bits = (BYTE*)FreeImage_GetBits(scaled);

	*width = FreeImage_GetWidth(scaled);
	*height = FreeImage_GetHeight(scaled);

	if((bits == 0) || (width == 0) || (height == 0)) {
		return 0;
	}

	FreeImage_Unload(dib);

	return bits;
}


int main(int argc, const char * argv[]) {

	NW = 200;
	NH = 200;
  dt = 0.2f;
  diff = 0.0f;
  visc = 0.00001f;
  force = 0.8f;
  source = 3.0f;
  
  int size = (NW+2)*(NH+2);//*(N+2); // cube
  
	u = (float *)malloc(size * sizeof(float));
	memset(u, 0, size * sizeof(float));

	v = (float *)malloc(size * sizeof(float));
	memset(v, 0, size * sizeof(float));

	u_prev = (float *)malloc(size * sizeof(float));
	memset(u_prev, 0, size * sizeof(float));

	v_prev = (float *)malloc(size * sizeof(float));
	memset(v_prev, 0, size * sizeof(float));

	dens = (float *)malloc(size * sizeof(float));
	memset(dens, 0, size * sizeof(float));

	dens_prev	= (float *)malloc(size * sizeof(float));
	memset(dens_prev, 0, size * sizeof(float));


//--

  color_r_u = (float *)malloc(size * sizeof(float));
  memset(color_r_u, 0, size * sizeof(float));

  color_r_v = (float *)malloc(size * sizeof(float));
  memset(color_r_v, 0, size * sizeof(float));

  color_r_u_prev = (float *)malloc(size * sizeof(float));
  memset(color_r_u_prev, 0, size * sizeof(float));

  color_r_v_prev = (float *)malloc(size * sizeof(float));
  memset(color_r_v_prev, 0, size * sizeof(float));

  color_r = (float *)malloc(size * sizeof(float));
  memset(color_r, 0, size * sizeof(float));

  color_r_prev = (float *)malloc(size * sizeof(float));
  memset(color_r_prev, 0, size * sizeof(float));

//--

  color_g_u = (float *)malloc(size * sizeof(float));
  memset(color_g_u, 0, size * sizeof(float));

  color_g_v = (float *)malloc(size * sizeof(float));
  memset(color_g_v, 0, size * sizeof(float));

  color_g_u_prev = (float *)malloc(size * sizeof(float));
  memset(color_g_u_prev, 0, size * sizeof(float));

  color_g_v_prev = (float *)malloc(size * sizeof(float));
  memset(color_g_v_prev, 0, size * sizeof(float));

  color_g = (float *)malloc(size * sizeof(float));
  memset(color_g, 0, size * sizeof(float));

  color_g_prev = (float *)malloc(size * sizeof(float));
  memset(color_g_prev, 0, size * sizeof(float));

  //--

  color_b_u = (float *)malloc(size * sizeof(float));
  memset(color_b_u, 0, size * sizeof(float));

  color_b_v = (float *)malloc(size * sizeof(float));
  memset(color_b_v, 0, size * sizeof(float));

  color_b_u_prev = (float *)malloc(size * sizeof(float));
  memset(color_b_u_prev, 0, size * sizeof(float));

  color_b_v_prev = (float *)malloc(size * sizeof(float));
  memset(color_b_v_prev, 0, size * sizeof(float));

  color_b = (float *)malloc(size * sizeof(float));
  memset(color_b, 0, size * sizeof(float));

  color_b_prev = (float *)malloc(size * sizeof(float));
  memset(color_b_prev, 0, size * sizeof(float));
  
  GLboolean running;

	FreeImage_Initialise(true);

	unsigned int width, height, bpp = 0;
	BYTE* imageData = LoadTexture(argv[1], &width, &height, &bpp);

  for (unsigned int i = 0; i < size; i++) {
		//dens[i] = 1.0f;

		int components = bpp / 8.0f;
		BYTE b = imageData[0+(i*components)];
		BYTE g = imageData[1+(i*components)];
		BYTE r = imageData[2+(i*components)];
		//BYTE a = imageData[3+(i*components)];

		if (!(i % (NW + 2))) {
			//printf("\n", r);
		}

		//printf("%03u ", r);
		
		color_r[i] = r / 255.0f;
		color_g[i] = g / 255.0f;
		color_b[i] = b / 255.0f;

//     if (i < (float)size / 2.0f) {
//       
//       color_r[i] = 126/255.0f;
//       color_g[i] = 11/255.0f;
//       color_b[i] = 128/255.0f;
//     }
  }

  // Initialize GLFW
  if(!glfwInit()) {
    fprintf( stderr, "Failed to initialize GLFW\n" );
    return EXIT_FAILURE;
  }
 
  // Open OpenGL window
  if(!glfwOpenWindow(WINDOW_WIDTH, WINDOW_HEIGHT, 0, 0, 0, 0, 0, 0, GLFW_WINDOW)) {
    fprintf(stderr, "Failed to open GLFW window\n");
    glfwTerminate();
    return EXIT_FAILURE;
  }

  GLenum err = glewInit();
  if (GLEW_OK != err) {
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
  }
  
  
  glfwSetKeyCallback(keyCallback);
  
  glfwSetWindowTitle("Navier Stokes");
  
  // Enable vertical sync (on cards that support it)
  glfwSwapInterval(1);
  
  GLuint shaderProgram = createShaderProgram();
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  forward = -1.0f;
  
  running = GL_TRUE;
  while (running) {
    
    input();
    update();
    
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    glUseProgram(shaderProgram);
    
    glm::mat4 projection = glm::perspective(45.0f, (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT, 0.1f, 100.0f);
    GLuint projectionLocation = glGetUniformLocation(shaderProgram, "Projection");
    glUniformMatrix4fv(projectionLocation, 1, false, glm::value_ptr(projection));
    
    glm::mat4 view(1.0f);
    view = glm::rotate(view, (yRotation / (float)M_PI) * 180.0f, glm::vec3(0, 1, 0));
    view = glm::translate(view, glm::vec3(right, up, forward));
  
    GLuint viewLocation = glGetUniformLocation(shaderProgram, "View");
    glUniformMatrix4fv(viewLocation, 1, false, glm::value_ptr(view));

    render();
    
    // Swap buffers
    glfwSwapBuffers();
    
    // Check if the ESC key was pressed or the window was closed
    running = !glfwGetKey(GLFW_KEY_ESC) && glfwGetWindowParam(GLFW_OPENED);
  }
  
  // Close OpenGL window and terminate GLFW
  glfwTerminate();
  
  return 0;
}
