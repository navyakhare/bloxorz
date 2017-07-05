#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;
void play()
{
  system("aplay Sounds/mono.wav &");
}
struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;

    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

struct Block
{
  VAO *block;
  VAO *frame;
  float x,y,z;
  short int rx,ry,rz;
  float rangle;
  float sangle;
  short int orientation;
};

typedef struct Tile
{
  VAO *tile;
  int type;
  float x, y, z;
  bool tfall;
  bool enabled;
  bool toggle;
}tileb;
bool istransition = false;

double timestarted, timeended;
bool fall = false;
int moves = 0;
float zoom = 1.0f;
int Board[13][13] = 
{
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,1,1,5,1,0},
    {0,0,0,0,0,0,0,0,1,2,2,1,0},
    {0,0,0,0,0,0,0,0,1,1,1,1,0},
    {0,0,0,0,0,0,0,0,0,0,4,0,0},   // Reference format a[y][x] -->  Y is the row number top to bottom (top = 0 ) X is the column number left to right (left = 0)
    {0,0,1,2,2,2,1,1,0,0,4,0,0},
    {0,0,1,3,2,2,1,1,4,4,1,0,0},
    {0,0,1,2,2,2,2,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0}
};

bool ts = false;
int switchloc[2] = {7,3};

tileb BoardObj[13][13];

struct Block player;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
//  fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    
    glfwDestroyWindow(window);
    glfwTerminate();
//    exit(EXIT_SUCCESS);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = green;
        color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}

/**************************
 * Customizable functions *
 **************************/

float freecamera_theta = 45;
float tpcamera_theta = 0;
float tpcamera_theta_old = 0;
float rectangle_rotation = 0;
int sx,sy,sz=1;
short int camera=0;
short int mcam=2;
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */

void keybindings(char A)
{
  if(A == 'U')
  {
    moves++;
    ts = false;
    if(player.orientation == 2)
    {
      istransition = true;
      player.sangle += 9;
      player.y += 0.20;
      sx = -1;
      sz = 0;
      sy = 0; 
    }
    else
    {
      player.rx = -1;
      player.rz = 0;
      player.ry = 0;
      player.rangle += 9;
      player.y += 0.30;
      if(player.orientation == 0)player.z -= 0.1;
      else if(player.orientation == 1)player.z += 0.1;
    }
  }
  else if(A == 'D')
  {
    moves++;
    ts = false;
    istransition = true;
    if(player.orientation == 2)
    {
      player.sangle += 9;
      player.y-=0.20;
      sx = 1;
      sz = 0;
      sy = 0; 
    }
    else
    {
      player.rx = 1;
      player.rz = 0;
      player.ry = 0;
      player.rangle += 9;
      player.y -= 0.30;
      if(player.orientation == 0) player.z -= 0.1;
      else if(player.orientation == 1) player.z += 0.1;
    }
  }
  else if(A == 'L')
  {
    moves++;
    ts = false;
    istransition = true;
    if(player.orientation == 1)
    {
      player.sangle += 9;
      player.x -= 0.20;
      sy = -1;
      sx = 0;
      sz = 0;
    }
    else
    {
      player.rx = 0;
      player.rz = 0;
      player.ry = -1;
      player.x -= 0.30;
      player.rangle += 9;
      if(player.orientation == 0)player.z -= 0.1;
      else if(player.orientation == 2)player.z += 0.1;
    }
  }
  else if(A == 'R')
  {
    moves++;
    ts = false;
    istransition = true;
    if(player.orientation == 1)
    {
      player.sangle += 9;
      player.x += 0.20;
      sy = 1;
      sx = 0;
      sz = 0;
    }
    else
    {
      player.rx = 0;
      player.rz = 0;
      player.ry = 1;
      player.rangle += 9;
      player.x += 0.30;
      if(player.orientation == 0)player.z -= 0.1;
      else if(player.orientation == 2)player.z += 0.1;
    }
  }
}

void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
  {
       // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE && !fall) {
      switch (key) {
        case GLFW_KEY_UP:
        {system("aplay -q Sounds/exp.wav &"); 
          if(camera!=1 && camera!=2) keybindings('U');
          else
          {
            tpcamera_theta_old = tpcamera_theta;
            if(fmod(fabs(tpcamera_theta),360) ==0) keybindings('U');
            else if((fmod(tpcamera_theta,360) == 90) or (fmod(tpcamera_theta,360) == -270)) keybindings('R');
            else if((fmod(tpcamera_theta,360) == 270) or (fmod(tpcamera_theta,360) == -90)) keybindings('L');
            else if((fmod(tpcamera_theta,360) == 180) or (fmod(tpcamera_theta,360) == -180)) keybindings('D');
          }
          mcam = 2;
          break;
        }
        case GLFW_KEY_DOWN:
        {system("aplay -q Sounds/exp.wav &"); 
          if(camera!=1 && camera!=2) keybindings('D');
          else
          {
            tpcamera_theta_old = tpcamera_theta;
            if(fmod(fabs(tpcamera_theta),360)==0) keybindings('D');
            else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('L');
            else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('R');
            else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('U');
          }
          tpcamera_theta -= 18;
          mcam = 0;
          break;
        }
        case GLFW_KEY_RIGHT:
        { system("aplay Sounds/exp.wav &"); 

          if(camera!=1 && camera!=2) keybindings('R');
          else
          {
            tpcamera_theta_old = tpcamera_theta;
            if (fmod(fabs(tpcamera_theta),360) == 0)keybindings('R');
            else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('D');
            else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('U');
            else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('L');
          }
          tpcamera_theta +=9;
          mcam = 1;
          break;
        }
        case GLFW_KEY_LEFT:
        {system("aplay -q Sounds/exp.wav &"); 

          if(camera!=1 && camera!=2) keybindings('L');
          else
          {
            tpcamera_theta_old = tpcamera_theta;
            if (fmod(fabs(tpcamera_theta),360) == 0)keybindings('L');
            else if((fmod(tpcamera_theta,360) == 90) || (fmod(tpcamera_theta,360) == -270)) keybindings('U');
            else if((fmod(tpcamera_theta,360) == 270) || (fmod(tpcamera_theta,360) == -90)) keybindings('D');
            else if((fmod(tpcamera_theta,360) == 180) || (fmod(tpcamera_theta,360) == -180)) keybindings('R');
          }
          mcam = -1;
          tpcamera_theta -= 9;
          break;
        }
        case GLFW_KEY_C:  
        system("aplay -q Sounds/laser.wav &");
        camera = (camera + 1)%4;
        default:
        break;
      }

    }
    else if (action == GLFW_PRESS) {
      switch (key) {
        case GLFW_KEY_Z:
          if(zoom < 1.49)
            zoom += 0.1;
          break;
        case GLFW_KEY_X:
          if(zoom > 0.51)
            zoom -= 0.1;
          break;
        case GLFW_KEY_A:
        system("aplay -q Sounds/laser.wav &");
        freecamera_theta += 10;
        break;
        case GLFW_KEY_D:
        system("aplay -q Sounds/laser.wav &");
        freecamera_theta -= 10;
        break;
        case GLFW_KEY_R:
        system("aplay -q Sounds/laser.wav &");
        freecamera_theta = 135;
        break;
        case GLFW_KEY_ESCAPE:
        system("aplay -q Sounds/laser.wav &");
        quit(window);
        break;
        default:
        break;

      }
    }
  }

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        default:
            break;
    }
}


/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 108.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    //Matrices.projection = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 500.0f);
}

static const GLfloat base_buffer_data [] = {
    -1,-1,-2, // vertex 1
    1,-1,-2, // vertex 2
    1, 1,-2, // vertex 3

    1, 1,-2, // vertex 3
    -1, 1,-2, // vertex 4
    -1,-1,-2,  // vertex 1

    -1,-1,-2,
    -1,-1,-2.5,
    -1,1,-2.5,

    -1,1,-2.5,
    -1,1,-2,
    -1,-1,-2,

    -1,1,-2.5,
    -1,1,-2,
    1,1,-2,

    1,1,-2,
    1,1,-2.5,
    -1,1,-2.5,

    1,1,-2,
    1,1,-2.5,
    1,-1,-2.5,

    1,-1,-2.5,
    1,-1,-2,
    1,1,-2,

    1,-1,-2.5,
    1,-1,-2,
    -1,-1,-2,

    -1,-1,-2,
    -1,-1,-2.5,
    1,-1,-2.5,

    -1,-1,-2.5, // vertex 1
    1,-1,-2.5, // vertex 2
    1, 1,-2.5, // vertex 3

    1, 1,-2.5, // vertex 3
    -1, 1,-2.5, // vertex 4
    -1,-1,-2.5  // vertex 1
};

static const GLfloat base_colour_data [] = {
    0.7,0.7,0.7, // vertex 1
    0.7,0.7,0.7, // vertex 2
    0.7,0.7,0.7, // vertex 3

    0.7,0.7,0.7, // vertex 1
    0.7,0.7,0.7, // vertex 2
    0.7,0.7,0.7, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2,

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.7,0.7,0.7, // vertex 1
    0.7,0.7,0.7, // vertex 2
    0.7,0.7,0.7, 

    0.7,0.7,0.7, // vertex 1
    0.7,0.7,0.7, // vertex 2
    0.7,0.7,0.7,  // vertex 1
};
static const GLfloat base_colour_data_fragile [] = {
    0.7,0.4,0.3, // vertex 1
    0.7,0.4,0.3, // vertex 2
    0.7,0.4,0.3, // vertex 3

    0.7,0.4,0.3, // vertex 1
    0.7,0.4,0.3, // vertex 2
    0.7,0.4,0.3, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2,

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.7,0.7,0.3, // vertex 1
    0.7,0.7,0.3, // vertex 2
    0.7,0.7,0.3, 

    0.7,0.7,0.3, // vertex 1
    0.7,0.7,0.3, // vertex 2
    0.7,0.7,0.3  // vertex 1
};
static const GLfloat base_colour_data_switch [] = {
    0.2,0.4,0.3, // vertex 1
    0.2,0.4,0.3, // vertex 2
    0.2,0.4,0.3, // vertex 3

    0.2,0.4,0.3, // vertex 1
    0.2,0.4,0.3, // vertex 2
    0.2,0.4,0.3, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2,

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.7,0.7,0.3, // vertex 1
    0.7,0.7,0.3, // vertex 2
    0.7,0.7,0.3, 

    0.7,0.7,0.3, // vertex 1
    0.7,0.7,0.3, // vertex 2
    0.7,0.7,0.3  // vertex 1
};

static const GLfloat base_colour_data_bridge [] = {
    0.2,0.4,0.7, // vertex 1
    0.2,0.4,0.7, // vertex 2
    0.2,0.4,0.7, // vertex 3

    0.2,0.4,0.7, // vertex 1
    0.2,0.4,0.7, // vertex 2
    0.2,0.4,0.7, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2,

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, 

    0.2,0.2,0.2, // vertex 1
    0.2,0.2,0.2, // vertex 2
    0.2,0.2,0.2, // vertex 3

    0.2,0.4,0.7, // vertex 1
    0.2,0.4,0.7, // vertex 2
    0.2,0.4,0.7, // vertex 3

    0.2,0.4,0.7, // vertex 1
    0.2,0.4,0.7, // vertex 2
    0.2,0.4,0.7  // vertex 1
};

void createBoard()
{
  for(int i = 0; i<13; i++)
  {
    for(int j = 0; j<13; j++)
    {
      BoardObj[i][j].z =0;
      if(Board[i][j] != 0)
      {
        if(Board[i][j] == 2)  
          BoardObj[i][j].tile = create3DObject(GL_TRIANGLES, 36, base_buffer_data, base_colour_data_fragile, GL_FILL);
        else if(Board[i][j] == 3)
        {
          BoardObj[i][j].toggle = false;
          BoardObj[i][j].tile = create3DObject(GL_TRIANGLES, 36, base_buffer_data, base_colour_data_switch, GL_FILL);
        }
        else if(Board[i][j] == 4)
        {
          BoardObj[i][j].enabled = false;
          BoardObj[i][j].tile = create3DObject(GL_TRIANGLES, 36, base_buffer_data, base_colour_data_bridge, GL_FILL);
        }
        else
          BoardObj[i][j].tile = create3DObject(GL_TRIANGLES, 36, base_buffer_data, base_colour_data, GL_FILL);

      }
    }
  }

}
// Creates the triangle object used in this sample code
void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -1,-1,-2, // vertex 1
    1,-1,-2, // vertex 2
    1, 1,-2, // vertex 3

    1, 1,-2, // vertex 3
    -1, 1,-2, // vertex 4
    -1,-1,-2,  // vertex 1

    -1,-1,2, // vertex 1
    1,-1,2, // vertex 2
    1, 1,2, // vertex 3

    1, 1,2, // vertex 3
    -1, 1,2, // vertex 4
    -1,-1,2,  // vertex 1

    -1,-1,-2, // vertex 1
    1,-1,-2, // vertex 2
    -1,-1,2, // vertex 1

    1,-1,2, // vertex 1
    -1,-1,2, // vertex 2
    1,-1,-2, // vertex 2

    -1,1,-2, // vertex 1
    1,1,-2, // vertex 2
    -1,1,2, // vertex 1

    1,1,2, // vertex 1
    -1,1,2, // vertex 2
    1,1,-2, // vertex 2

    1,-1,-2, // vertex 2
    1, 1,-2, // vertex 3
    1,-1,2, // vertex 2

    1,1,2, // vertex 2
    1,-1,2, // vertex 3
    1,1,-2, // vertex 2

    -1,-1,2, // vertex 1
    -1,-1,-2,  // vertex 1
    -1, 1,2, // vertex 4

    -1,1,-2, // vertex 1
    -1,1,2,  // vertex 1
    -1,-1,-2 // vertex 4 
  };

  static const GLfloat color_buffer_data1 [] = {
    0.6,0.3,0.3, // color 1
    0.6,0.3,0.3, // color 2
    0.6,0.3,0.3, // color 3

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 1
    0.6,0.3,0.3, // color 2
    0.6,0.3,0.3, // color 3

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3,  // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3, // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3, // color 1

    0.6,0.3,0.3, // color 3
    0.6,0.3,0.3, // color 4
    0.6,0.3,0.3 // color 1
  };

  static const GLfloat color_buffer_data2 [] = {
    0.2,0.2,0.2, // color 1
    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 2

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 1
    0.2,0.2,0.2, // color 2
    0.2,0.2,0.2, // color 3

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3 
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2,  // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2, // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2, // color 1

    0.2,0.2,0.2, // color 3
    0.2,0.2,0.2, // color 4
    0.2,0.2,0.2 // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  player.block = create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data1, GL_FILL);

  player.frame =  create3DObject(GL_TRIANGLES, 36, vertex_buffer_data, color_buffer_data2, GL_LINE);
}

float camera_rotation_angle = 90;
float triangle_rotation = 0;


void checkfall()
{
  if(player.orientation == 0)
  {
    float j = (player.x + 12)/2.0;
    float i = (player.y + 12)/2.0;
    if(Board[(int)i][(int)j] == 0)
    {
      fall =true;
      return;
    }
  }
  glm::mat4 translateRectangle = glm::translate (glm::vec3(player.x , player.y , player.z));        // glTranslatef
}

void detectSwitch()
{
  if(player.orientation == 0)
  {
    int x = round((player.x + 12)/2.0);
    int y = round((player.y + 12)/2.0);
    if(Board[y][x] == 3)
    {
//      cout <<"test"<<endl;
      if(!ts)
      {
        if(BoardObj[switchloc[0]][switchloc[1]].toggle == true)
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = false;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = false;
        }
        else
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = true;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = true;
        }
        ts = true;
      }
    }
  }
  else if(player.orientation == 1)
  {
    int x = round((player.x + 12.0)/2.0);
    int y1 = round((player.y + 11.0)/2.0);
    int y2 = round((player.y + 13.0)/2.0);
    if(Board[y1][x] == 3)
    {
      if(!ts)
      { 
        if(BoardObj[switchloc[0]][switchloc[1]].toggle == true)
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = false;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = false;
        }
        else
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = true;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = true;
        }
        ts = true;
      }
    }
    else if(Board[y2][x] == 3)
    {
      if(!ts)
      {
        if(BoardObj[switchloc[0]][switchloc[1]].toggle == true)
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = false;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = false;
        }
        else
        {system("aplay -q Sounds/laser.wav &");
          for(int i= 0; i<13; i++)
          {
            for(int j= 0; j<13; j++)
            {
              if(Board[i][j] == 4)
                BoardObj[i][j].enabled = true;
            }
          }
          BoardObj[switchloc[0]][switchloc[1]].toggle = true;
        }
        ts = true;
      }
    }

  }
  else if(player.orientation == 2)
  {
    if(!ts)
    {
      ts = true;
    }

  }
}
bool game = true;

void wingame()
{
  system("aplay Sounds/tada.wav &");
  game = false;
  timeended = glfwGetTime();
  cout << endl << "----- GAME WON -----" << endl << endl;
  cout << "Time Taken : " << timeended - timestarted << "s"<< endl;
  cout << "Total number of moves : " << moves << endl;
  cout << endl << "--------------------" << endl;
}

void losegame()
{
  play();
  game = false;
  timeended = glfwGetTime();
  cout << endl << "----- GAME LOST -----" << endl << endl;
  cout << "Time Taken : " << timeended - timestarted << "s" << endl;
  cout << "Total number of moves : " << moves << endl;
  cout << endl << "--------------------" << endl;
}
/* Render the scene with openGL */
/* Edit this function according to your assignment */
void draw ()
{
  // clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  if(mcam == 0 && tpcamera_theta_old - tpcamera_theta != 180) tpcamera_theta -= 18;
  else if(mcam == 1 && tpcamera_theta -  tpcamera_theta_old != 90) tpcamera_theta += 9;
  else if(mcam == -1 && tpcamera_theta_old - tpcamera_theta != 90) tpcamera_theta -= 9;

  // Eye - Location of camera. Don't change unless you are sure!!
  if(camera == 0)
  {
    // Eye - Location of camera. Don't change unless you are sure!!
    glm::vec3 eye ( -15*cos(freecamera_theta*M_PI/180.0f)*zoom, -15*sin(freecamera_theta*M_PI/180.0f)*zoom, 15);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (player.x, player.y, player.z);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 0, 1);

    Matrices.view = glm::lookAt( eye, target, up );
  }

  else if (camera == 1)
  {
    glm::vec3 eye ( player.x - 6*sin(tpcamera_theta*M_PI/180.0f), player.y - 6*cos(tpcamera_theta*M_PI/180.0f), 8);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (player.x + 6*sin(tpcamera_theta*M_PI/180.0f) , player.y + 6*cos(tpcamera_theta*M_PI/180.0f), player.z - 2);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 0, 1);

    Matrices.view = glm::lookAt( eye, target, up ); 
  }
  else if(camera == 2)
  {
    glm::vec3 eye ( player.x + 2*sin(tpcamera_theta*M_PI/180.0f), player.y + 2*cos(tpcamera_theta*M_PI/180.0f),player.z + 2);
    // Target - Where is the camera looking at.  Don't change unless you are sure!!
    glm::vec3 target (player.x + 6*sin(tpcamera_theta*M_PI/180.0f) , player.y + 6*cos(tpcamera_theta*M_PI/180.0f), player.z - 4);
    // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
    glm::vec3 up (0, 0, 1);

    Matrices.view = glm::lookAt( eye, target, up ); 
  }
  else if(camera == 3)
  {
    Matrices.view = glm::lookAt(glm::vec3(-1,-1,15), glm::vec3(-1,-1,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane
  }
  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix

    for(int i = 0; i<13; i++)
    {
      for(int j = 0; j<13; j++)
      {
        if(Board[i][j] != 0)
        {
          Matrices.model = glm::mat4(1.0f);
          glm::mat4 translation = glm::translate(glm::vec3(j*2 -12, i*2 -12,BoardObj[i][j].z));
          Matrices.model *= translation;
          MVP = VP * Matrices.model;

          glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
          if((Board[i][j] == 4) && (BoardObj[switchloc[0]][switchloc[1]].toggle))
          {
            draw3DObject(BoardObj[i][j].tile);
          }
          else if(Board[i][j] != 4 && Board[i][j]!=5)
            draw3DObject(BoardObj[i][j].tile);
        }
      }
    }



    Matrices.model = glm::mat4(1.0f);

    glm::mat4 translateRectangle = glm::translate (glm::vec3(player.x , player.y , player.z));        // glTranslatef
    glm::mat4 rotateRectangle1 = glm::rotate((float)(player.rangle*M_PI/180.0f), glm::vec3(player.rx,player.ry,player.rz)); // rotate about vector (-1,1,1)
    glm::mat4 rotateRectangle2 = glm::rotate((float)(player.sangle*M_PI/180.0f), glm::vec3(sx, sy, sz));
    Matrices.model *= (translateRectangle * rotateRectangle2 *rotateRectangle1);
    MVP = VP * Matrices.model;
    glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(player.block);
    draw3DObject(player.frame);

      if(fmod(player.sangle,10)!=0)
      {
        player.sangle+=9;
        if(sx==-1)player.y+=0.20;
        else if(sx==1)player.y-=0.20;
        else if(sy==-1)player.x-=0.20;
        else if(sy==1)player.x+=0.20;
      }
      else
      {
        player.sangle = 0;
          if(fmod(player.rangle,10)==0)
          {
            
            if(player.orientation == 0)
            {
              int pox = round((float)(player.x + 12.0)/2.0);
              int poy = round((float)(player.y + 12.0)/2.0);
              detectSwitch();
              if(Board[poy][pox] == 0|| (Board[poy][pox] ==4 && !BoardObj[poy][pox].enabled))
              {
                fall = true;
                player.z -= 0.5;
                if(game)
                  losegame();
              }
              else if(Board[poy][pox] == 2)
              {
                fall = true;
                BoardObj[poy][pox].tfall = true;
                player.z -=0.5;
                BoardObj[poy][pox].z -= 0.5;
                if(game)
                  losegame();
              }
              else if(Board[poy][pox] == 5)
              {
                fall= true;
                BoardObj[poy][pox].tfall = true;
                player.z -=0.5;
                BoardObj[poy][pox].z -= 0.5;
                if(game)
                  wingame();
              }
            }

            else if(player.orientation == 1)
            {
              int pox = round((float)(player.x +12.0)/2.0);
              int poy1 = round((float)(player.y +11.0)/2.0) ;
              int poy2 =  round((float)(player.y +13.0)/2.0) ;

              if((Board[poy1][pox] == 0 && Board[poy2][pox] == 0) || (Board[poy1][pox] == 0 && Board[poy2][pox] == 4 && !BoardObj[poy2][pox].enabled) || (Board[poy2][pox] == 0 && Board[poy1][pox] == 4 && !BoardObj[poy1][pox].enabled) || (Board[poy2][pox] == 4 && Board[poy1][pox] == 4 && !BoardObj[poy1][pox].enabled))
              { 
                fall = true;
                player.z -=0.5;
                if(game)
                  losegame();
              }
              else if(Board[poy1][pox] == 0 || (Board[poy1][pox] == 4 && !BoardObj[poy1][pox].enabled))
              {
                player.rx = 1;
                player.ry = 0;
                player.rz = 0;
                player.z -= 0.5;
                player.rangle += 9;
                if(game)
                  losegame();
              }
              else if(Board[poy2][pox] == 0|| (Board[poy2][pox] == 4 && !BoardObj[poy2][pox].enabled) )
              {
                player.rx = -1;
                player.ry =0;
                player.rz = 0;
                player.z-=0.5;
                player.rangle += 9;
                if(game)
                  losegame();
              }
              
             }
            else if(player.orientation == 2)
            {

              int pox1 = round((float)(player.x +11.0)/2.0) ;
              int pox2 = round((float)(player.x +13.0)/2.0) ;
              int poy =  round((float)(player.y +12.0)/2.0) ;
              if((Board[poy][pox1] == 0 && Board[poy][pox2] == 0) || (Board[poy][pox1] == 0 && Board[poy][pox2] == 4 && !BoardObj[poy][pox2].enabled) || (Board[poy][pox2] == 0 && Board[poy][pox1] == 4 && !BoardObj[poy][pox1].enabled) || (Board[poy][pox2] == 4 && Board[poy][pox1] == 4 && !BoardObj[poy][pox1].enabled))
              {
                fall = true;
                player.z -=0.5;
                if(game)
                  losegame();
              }
              else if(Board[poy][pox1] == 0 || (Board[poy][pox1] == 4 && !BoardObj[poy][pox1].enabled))
              {                
                //player.x -= 2;
                player.rx = 0;
                player.ry = -1;
                player.rz = 0;
                player.z -= 0.5;
                player.rangle += 9;
                if(game)
                  losegame();
              }
              else if(Board[poy][pox2] == 0 || (Board[poy][pox2] == 4 && !BoardObj[poy][pox2].enabled))
              {
                player.rx = 0;
                player.ry = 1;
                player.rz = 0;
                player.z -= 0.5;
                player.rangle += 9;
                if(game)
                  losegame();
              }
            }
          }



          else
          {
            player.rangle+=9;
                
              if(player.rx == -1)
              {
                  player.y += 0.30;
                  if(player.orientation == 0)
                  {
                      player.z -= 0.1;
                      if(fmod(player.rangle,10)==0) 
                      {
                          istransition = false;
                          player.orientation = 1;
                      }
                  }
                  else if(player.orientation == 1)
                  {
                      player.z += 0.1;
                      if(fmod(player.rangle,10)==0) 
                      {
                        istransition = false;
                        player.orientation = 0;
                      }
                  }
              }
              else if(player.rx == 1)
              {
                  player.y -= 0.30;
                if(player.orientation == 0)
                {
                      player.z -= 0.1;
                      if(fmod(player.rangle,10)==0)
                      {
                        istransition = false;
                        player.orientation = 1;
                      }
                }
                  else if(player.orientation == 1)
                  {
                      player.z += 0.1;
                      if(fmod(player.rangle,10)==0) 
                      {
                        istransition = false;
                        player.orientation = 0;
                      }
                  }
              }

              if(player.ry == -1)
                {
                 player.x -= 0.30;
                  if(player.orientation == 0)
                    {
                      player.z -= 0.1;
                      if(fmod(player.rangle,10)==0) 
                      {
                        istransition = false;
                        player.orientation = 2;
                      }
                    }
                  else if(player.orientation == 2)
                   {
                      player.z += 0.1;
                      if(fmod(player.rangle,10)==0)
                      {
                        istransition = false;
                        player.orientation = 0;
                      }
                    }
                }
              else if(player.ry == 1)
                {
                  player.x += 0.30;
                  if(player.orientation == 0)
                    {
                      player.z -= 0.1;
                      if(fmod(player.rangle,10)==0)
                      {                  
                        istransition = false;
                        player.orientation = 2;
                      }
                    }
                  else if(player.orientation == 2)
                    {
                      player.z += 0.1;
                      if(fmod(player.rangle,10)==0) 
                      {
                        istransition = false;
                        player.orientation = 0;
                      }
                  }
            }
          }
        }
    

  // Increment angles
  //float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  //triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  //rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}

/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
//        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 16);


    window = glfwCreateWindow(width, height, "Blockorz", NULL, NULL);

    if (!window) {
        glfwTerminate();
//        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle ();
	createBoard();
  timestarted = glfwGetTime();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (0.9f, 0.9f, 0.9f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);
  glEnable(GL_MULTISAMPLE);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 800;
  int height = 800;
  player.rangle = 0;
  player.sangle = 0;
  player.rz = 1;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) {

        // OpenGL Draw commands
        draw();

        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            last_update_time = current_time;
        }
    }

    glfwTerminate();
//    exit(EXIT_SUCCESS);
}
