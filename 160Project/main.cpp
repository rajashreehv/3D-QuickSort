

#include "Angel.h"
#include "Mesh.h"
#include "Animation.h"
#include "Camera.h"
#include "QuickSortVisual.h"
#include "glm/glm.hpp"
#include "FreeImage.h"
#include <Windows.h>

#include <cmath>
#include <utility>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <string>
#include <stack>
#include <list>
#include <AntTweakBar.h>

int WINDOW_BORDER;
const int HEIGHT = 800;
int winWidth;
int winHeight;

typedef enum {SCALE=1, ROTATE_V, ROTATE_H, TRANSLATE} TransformMode;
Camera* globalCamera;
TransformMode currentTransform;
//Initialize the static variable.
unsigned char Mesh::globalColorID[3] = {0,0,0};

//Allow us to create a menu for switching between diffuse, ambient, and ambient + diffuse lighting.
const LightingType Lighting_MAX = COLOR_ID;
LightingType currentLighting = NORMAL_MODE;
LightingType prevLighting = NORMAL_MODE;
float translate = 0.0;
bool zoomMode;

Mesh* selected;
int pauseMenuValue = 0;
QuickSortVisual* visualization;

void display();

//comparator for findmax and findmin.
inline bool compareZ(glm::vec3 &a, glm::vec3 &b) { return a.z < b.z; }

//Takes a range and produces a random value whithin that range.
float randomRanged(float a, float b) {
	float random = ((float) rand()) / (float) RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}

void init()
{
	//globalCamera = glm::perspective(35.0f, 1.0f, 0.01f, 200.0f); 
	glm::mat4 ortho = glm::ortho(-0.5f, 0.5f, -0.5f, 0.5f, 0.01f, 100.0f);
	globalCamera = new Camera(ORTHOGRAPHIC, ortho);
	globalCamera->moveTo(glm::vec3(1.0f, 1.0f, 1.0f));
	//Questions: Should length of array be limited?
	std::vector<int> array;
	array.push_back(1);
	array.push_back(4);
	array.push_back(2);
	array.push_back(5);
	array.push_back(9);
	array.push_back(3);
	array.push_back(7);

	visualization = new QuickSortVisual(array, globalCamera);
	
	glClearColor(1.0, 1.0, 1.0, 1.0);
}

//---------Glut Callback Functions------------
void display()
{
	int time = glutGet(GLUT_ELAPSED_TIME);
	if(currentLighting != prevLighting){
		for(auto i = visualization->getObjects()->begin(); i < visualization->getObjects()->end(); i++)
			(*i)->setLighting(currentLighting);
		prevLighting = currentLighting;
	}
	visualization->update(time);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
	glClearDepth(1.0);

	visualization->draw();
	TwDraw();
	glutSwapBuffers();
	glutPostRedisplay();
}

glm::vec4 viewPort;
glm::vec3 objCoordsNew, objCoordsOld;
glm::vec3 winCoordNew, winCoordsOld;
int prevX;
int prevY;
GLfloat depth;
//Uses color picking.
void mouseSelect(int button, int state, int x, int y){
	//Send event to atb. If it didn't handle it, then call our own code.
	if(!TwEventMouseButtonGLUT(button, state, x, y)){
		zoomMode = false;
		selected = NULL;
		zoomMode = false;
		prevX = x;
		prevY = winHeight - y - 1;
		if(state != GLUT_DOWN) //don't do anything if mouse is up.
			return;
		if(button == GLUT_RIGHT_BUTTON){
			zoomMode = true;
			return;
		}
		//else if(button = GLUT_LEFT_BUTTON) 

		unsigned char color[3];
		//Turn off lighting.
		for(auto i = visualization->getObjects()->begin(); i < visualization->getObjects()->end(); i++)
			(*i)->setLighting(COLOR_ID);
		
		//Render the visualization->getObjects()->
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
		glClearDepth(1.0);
		for(auto i = visualization->getObjects()->begin(); i < visualization->getObjects()->end(); i++)
			(*i)->draw();
		//Read color of mouse coordinate.
		glReadPixels(x, winHeight - y - 1, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, color);
		//We now use depth for translation because perspective makes farther things smaller.
		glReadPixels(x, winHeight - y - 1, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
		for(auto i = visualization->getObjects()->begin(); i < visualization->getObjects()->end(); i++){
			if((*i)->colorMatch(color))
				selected = *i;
		}
		if(selected != NULL){
			//iniitalize "previous"
			winCoordsOld = glm::vec3(prevX, prevY, depth);
			objCoordsOld = globalCamera->windowToWorld(winCoordsOld, viewPort);
			viewPort = glm::vec4(0.0f, 0.0f, winWidth, winHeight);
		}
		//Restore lighting.
		for(auto i = visualization->getObjects()->begin(); i < visualization->getObjects()->end(); i++)
			(*i)->setLighting(currentLighting);
	}
}


void reshape(int width, int height)
{
	winWidth = width;
	WINDOW_BORDER = HEIGHT - height;
	winHeight = height;
	glViewport(0, 0, width,height);
	TwWindowSize(width, height);
}

void keyboard(unsigned char key, int x, int y)
{
	if(!TwEventKeyboardGLUT(key, x, y)){
		switch (key) {
		case 's':
			//currentTransform = SCALE;
			visualization->stepOnce();
			break;
		case 't':
			currentTransform = TRANSLATE;
			break;
		case 'v':
			currentTransform = ROTATE_V;
			break;
		case 'h':
			currentTransform = ROTATE_H;
			break;
		}
	}
}

float direction;
void passiveMouse(int x, int y){
	TwEventMouseMotionGLUT(x, y);
}

const float SCALE_FACTOR = .02f;
const float ROTATION_FACTOR = 3.0f;

void activeMouse(int x, int y){
	if(!TwEventMouseMotionGLUT(x, y)){
		float differenceY = (float)(winHeight - y - 1 - prevY);
		float differenceX = (float)(prevX - x);
	float scale, rotation;
		bool positive = true;
		if(zoomMode == true){
			float zoomFactor;
			if(differenceY < 0.0f){
				zoomFactor = .95;
			}
			else {
				zoomFactor = 1.05f;
			}
				globalCamera->scale(zoomFactor);
		}
		else if(selected != NULL){
			switch(currentTransform){
				case SCALE:
					scale = 1.0f - SCALE_FACTOR;
					if(differenceY > 0.0f)
						scale = SCALE_FACTOR + 1.0f;
					differenceY = abs(differenceY);
					if(differenceY > 1.0)
						selected->scaleCenterUniform(scale);
					break;
				case TRANSLATE:
						winCoordNew = glm::vec3(x, winHeight - y - 1, depth);
						objCoordsNew = globalCamera->windowToWorld(winCoordNew, viewPort);
						// std::cout << "xPrev: " << prevX << ", " << prevY << std::endl;
						// std::cout << objCoordsNew.x << " " << objCoordsNew.y << std::endl;
						selected->translate(glm::vec3(objCoordsNew.x - objCoordsOld.x, objCoordsNew.y - objCoordsOld.y, 0.0f));
						objCoordsOld = objCoordsNew;
					break;
				case ROTATE_V:
					rotation = ROTATION_FACTOR;
					if(differenceX < 0.0f){
						positive = false;
					}
						selected->rotateSelf(glm::vec3(0.0f, rotation, 0.0f), positive);
					break;
				case ROTATE_H:
					rotation = ROTATION_FACTOR;
					if(differenceY < 0.0f){
						positive = false;
					}
						selected->rotateSelf(glm::vec3(rotation, 0.0f, 0.0f), positive);
					break;
			}
		}
		prevX = x;
		prevY = winHeight - y - 1;
	}
}

void Terminate()
{ 
	TwTerminate();
}

//--Menu Callback Functions--
void TW_CALL SetPauseCB(const void *value, void *clientData){
	(void)clientData;
	pauseMenuValue = *(const int *)value;
	if( pauseMenuValue!= 0 ) {
		 visualization->setPause(true);
	}
	else{
		visualization->setPause(false);
	}
}

void TW_CALL GetPauseCB(void *value, void *clientData){
	(void)clientData;
	*(int *)value = pauseMenuValue;
}

int main(int argc, char **argv)
{
	srand ( unsigned ( std::time(0) ) );
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(800, HEIGHT);
	glutCreateWindow("160 Project Prototype");
	glutDisplayFunc(display);
	glutMouseFunc(mouseSelect);
	//glutCreateMenu(NULL);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	//Allows alpha transparency blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable (GL_TEXTURE_2D);
	glewExperimental = GL_TRUE;
	glewInit();

	//glutMouseFunc((GLUTmousebuttonfun)TwEventMouseButtonGLUT);
	glutMotionFunc(activeMouse);
	glutPassiveMotionFunc(passiveMouse);
	glutSpecialFunc((GLUTspecialfun)TwEventSpecialGLUT);
	glutKeyboardFunc(keyboard);
	atexit(Terminate);
	//TwGLUTModifiersFunc(glutGetModifiers);
	glutReshapeFunc(reshape);
	
	//Initialize AntTweakBar
	TwInit(TW_OPENGL, NULL);
	TwBar* bar = TwNewBar("OptionBar");
	TwDefine(" OptionBar position='0 10' size='220 300' color='25 113 255' label='Controls' ");

	TwAddVarCB(bar, "Pause", TW_TYPE_BOOL32, SetPauseCB, GetPauseCB, NULL, " key='space' help='Pause' ");
	/*{
		TwEnumVal lightingEV[Lighting_MAX] = { {NORMAL_MODE , "Normal"}, {COLOR_ID, "Color ID"} };
		TwType lightingEnum = TwDefineEnum("LightingType", lightingEV, Lighting_MAX);
		TwAddVarRW(bar, "Lighting", lightingEnum, &currentLighting, " keyIncr=',' keyDecr='.' help='Select Lighting Type.' ");
	}
	*/

	init();
	glutMainLoop();
	return 0;
}


/*
Really helpful for learning about managing array visualization->getObjects()->
https://developer.apple.com/library/ios/#documentation/3DDrawing/Conceptual/OpenGLES_ProgrammingGuide/TechniquesforWorkingwithVertexData/TechniquesforWorkingwithVertexData.html
http://en.wikibooks.org/wiki/OpenGL_Programming/Bounding_box
*/