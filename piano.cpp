#include "GL/glew.h"
#include "GL/freeglut.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <stdio.h>
#include "include/irrKlang.h"
#include "tga.h"
#include "shaderprogram.h"
#include <vector>
#include <iostream>		
#include <string>
#include <sstream>
#include "RtMidi.h"
#pragma comment(lib, "irrKlang.lib")

using namespace std;
using namespace irrklang;
using namespace glm;

struct Model {
	float *v;
	float *vt;
	float *vn;
	float *colors;
	int vertexCount;
	GLuint vao;
	GLuint bufVertices; 
	GLuint bufColors;
	GLuint bufNormals; 
	GLuint bufTexCoords;
	bool visible=true;
	mat4  modelMatrix;
	float offset=0;
	bool animateDown = false;
	bool animationOn = false;
	bool animateUp = false;
	int animationCounter=0;
	int type;
	int nr;
	bool openCloseCover = false;
	GLuint *texPointer;
};

ISoundEngine* engine[88];
ISoundSource* snd[88];
string klawisze = "`1234567890-=qwertyuiop[]asdfghjkl;'zxcvbnm,./";
vector<unsigned char> MIDImessage;
RtMidiIn *MIDIin = new RtMidiIn();

bool sustain=false;
int i;
float off=0.2438f;
Model piano[12],key[88],mechA[88],mechB[88];
float cameraX = -2.0f;
float cameraY = 2.0f;
float cameraDistance = 3.0f;

mat4  matP, matV, matM;

int windowPositionX=100;
int windowPositionY=100;
int windowWidth=1000;
int windowHeight=1000;
float cameraAngle=50.0f;

ShaderProgram *shaderProgram; 

GLuint texWood, texBlack, texBrass, texBrightWood, texCopper, texWhite, texRed;

int countElements(const char * path){

	FILE * file = fopen(path, "r");
	char line[300];
	int count = 0;

	while (fscanf(file, "%s", line) != EOF){
		if (strcmp(line, "f") == 0) count++;
	}
	return count;
}

void cleanShaders() {
	delete shaderProgram;
}

void freeVBO() {
	for (int i = 0; i < 12; i++){
		glDeleteBuffers(1, &piano[i].bufVertices);
		glDeleteBuffers(1, &piano[i].bufColors);
		glDeleteBuffers(1, &piano[i].bufNormals);
		glDeleteBuffers(1, &piano[i].bufTexCoords);
	}
}

void freeVAO() {
	for (int i = 0; i < 12; i++){
		glDeleteVertexArrays(1, &piano[i].vao);
	}
}

void playSound(int nr){
	engine[nr]->play2D(snd[nr]);
}

bool loadOBJ(const char * path,float *vertices,float *texCoords,float *normals){

	int c = 0;
	std::vector< unsigned int > vertexIndices, uvIndices, normalIndices;
	std::vector< vec3 > temp_vertices;
	std::vector< vec2 > temp_uvs;
	std::vector< vec3 > temp_normals;

	int vertexIndex[3], uvIndex[3], normalIndex[3];
	char line[300];

	cout << path << endl;

	FILE * file = fopen(path, "r");
	if (file == NULL){
		printf("Blad odczytu OBJ\n");
		return false;
	}

	while (fscanf(file, "%s", line) != EOF){

		c++;

		if (strcmp(line, "v") == 0){
			vec3 vertex;
			fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			temp_vertices.push_back(vertex);
		}
		else if (strcmp(line, "vt") == 0){
			vec2 uv;
			fscanf(file, "%f %f\n", &uv.x, &uv.y);
			temp_uvs.push_back(uv);
		}
		else if (strcmp(line, "vn") == 0){
			vec3 normal;
			fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
			temp_normals.push_back(normal);
		}
		else if (strcmp(line, "f") == 0){
			string vertex1, vertex2, vertex3;
			int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2]);
			if (matches != 9){
				printf("Format pliku nieteges");
				return false;
			}

			vertexIndices.push_back(vertexIndex[0]);
			vertexIndices.push_back(vertexIndex[1]);
			vertexIndices.push_back(vertexIndex[2]);
			uvIndices.push_back(uvIndex[0]);
			uvIndices.push_back(uvIndex[1]);
			uvIndices.push_back(uvIndex[2]);
			normalIndices.push_back(normalIndex[0]);
			normalIndices.push_back(normalIndex[1]);
			normalIndices.push_back(normalIndex[2]);

		}
	}

	c = 0;
	for (unsigned int i = 0; i < vertexIndices.size(); i++){
		unsigned int vertexIndex = vertexIndices[i];
		vec3 vertex = temp_vertices[vertexIndex - 1];
		vertices[c] = vertex.x;
		vertices[c+1] = vertex.y;
		vertices[c+2] = vertex.z;
		vertices[c+3] = 1.0f;
		c += 4;	
	}
	c = 0;
	for (unsigned int i = 0; i < uvIndices.size(); i++){
		unsigned int uvIndex = uvIndices[i];
		vec2 uv = temp_uvs[uvIndex - 1];
		texCoords[c] = uv.x;
		texCoords[c + 1] = uv.y;
		c += 2;
	}
	c = 0;
	for (unsigned int i = 0; i < normalIndices.size(); i++){
		unsigned int normalIndex = normalIndices[i];
		vec3 normal = temp_normals[normalIndex - 1];
		normals[c] = normal.x;
		normals[c+1] = normal.y;
		normals[c+2] = normal.z;
		normals[c + 3] = 0.0f;
		c += 4;
	}
	fclose(file);
}

void drawObject(Model *model) {

	glUniform1i(shaderProgram->getUniformLocation("textureMap"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, *(model->texPointer));

	glBindVertexArray(model->vao);

	if (model->type == 1){
		if (model->animateDown && model->animationOn){
			model->animationCounter++;
			if (model->animationCounter <= 8){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 0.9370f, 0));
				model->modelMatrix = rotate(model->modelMatrix, 0.225f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -0.9370f, 0));
			}
			else {
				model->animationCounter = 0;
				model->animateDown = false;
				}
			if (model->animationCounter == 1) {
			mechA[model->nr].animateDown = true;
			mechA[model->nr].animationOn = true;
			}
		}

		else if (model->animateUp && !model->animateDown && model->animationOn){
			model->animationCounter++;

			if (model->animationCounter <= 8){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 0.9370f, 0));
				model->modelMatrix = rotate(model->modelMatrix, -0.225f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -0.9370f, 0));
			}
			else {
				model->animationCounter = 0;
				model->animateUp = false;
				model->animationOn = false;
				if (!sustain) engine[model->nr]->stopAllSounds();
			}
			if (model->animationCounter == 1) mechA[model->nr].animateUp = true;
		}
	}

	if (model->type == 2){
		if (model->animateDown && model->animationOn){
			model->animationCounter++;
			if (model->animationCounter <= 6){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 1.01f, -0.08f));
				model->modelMatrix = rotate(model->modelMatrix, -1.0f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -1.01f, 0.08f));
			}
			else {
				model->animationCounter = 0;
				model->animateDown = false;
			}
			if (model->animationCounter == 1) mechB[model->nr].animateDown = true;
		}

		else if (model->animateUp && !model->animateDown && model->animationOn){
			model->animationCounter++;

			if (model->animationCounter <= 6){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 1.01f, -0.08f));
				model->modelMatrix = rotate(model->modelMatrix, 1.0f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -1.01f, 0.08f));
			}
			else {
				model->animationCounter = 0;
				model->animateUp = false;
				model->animationOn = false;
			}
		}
	}

	if (model->type == 3){
		if (model->animateDown){
			model->animationCounter++;
			if (model->animationCounter <= 8){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 1.1175f, -0.1382f));
				model->modelMatrix = rotate(model->modelMatrix, -1.725f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -1.1175f, 0.1382f));
			}
			else {
				model->animationCounter = 0;
				model->animateDown = false;
				model->animateUp = true;
				playSound(model->nr);
				
			}
		}

		else if (model->animateUp && !model->animateDown){
			model->animationCounter++;

			if (model->animationCounter <= 8){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 1.1175f, -0.1382f));
				model->modelMatrix = rotate(model->modelMatrix, 1.725f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -1.1175f, 0.1382f));
			}
			else {
				model->animationCounter = 0;
				model->animateUp = false;
			}
		}
	}

	if (model->openCloseCover){
		model->animationCounter++;
		if ((model->animationCounter <= 30 && model == &piano[1]) || (model->animationCounter <= 15 && model == &piano[5])){
			if (model == &piano[5] && !model->animationOn){ //CTRL
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 0.0655f, -0.0495f));
				if (model->animateDown) model->modelMatrix = rotate(model->modelMatrix, 1.2f, vec3(1, 0, 0)); else model->modelMatrix = rotate(model->modelMatrix, -1.2f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -0.0655f, 0.0495f));
			}
			else if (model == &piano[5] && model->animationOn){ //ALT
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 0.0655f, -0.0495f));
				if (!model->animateDown) model->modelMatrix = rotate(model->modelMatrix, 1.2f, vec3(1, 0, 0)); else model->modelMatrix = rotate(model->modelMatrix, -1.2f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -0.0655f, 0.0495f));
			}
			else if (model == &piano[1]){
				model->modelMatrix = translate(model->modelMatrix, vec3(0, 0.9996f, 0.1232f));
				if (model->animateDown) model->modelMatrix = rotate(model->modelMatrix, 3.76f, vec3(1, 0, 0)); else model->modelMatrix = rotate(model->modelMatrix, -3.76f, vec3(1, 0, 0));
				model->modelMatrix = translate(model->modelMatrix, vec3(0, -0.9996f, -0.1232f));
			}
		} else {
			model->openCloseCover = false;
			model->animationCounter = 0;
			if ((model->animationOn && !model->animateDown)) { 
				model->animateDown = true;
				model->openCloseCover = false;
				model->animationOn = false;
			}
			else if (model->animationOn && model->animateDown){
				model->animateDown = false;
				model->openCloseCover = true;
			}
		}
	}

	matM = model->modelMatrix;

	glUniformMatrix4fv(shaderProgram->getUniformLocation("P"),1, false, value_ptr(matP));
	glUniformMatrix4fv(shaderProgram->getUniformLocation("V"),1, false, value_ptr(matV));
	glUniformMatrix4fv(shaderProgram->getUniformLocation("M"),1, false, value_ptr(matM));
	
	if (model->visible) glDrawArrays(GL_TRIANGLES, 0, model->vertexCount);
	
}


void assignVBOtoAttribute(char* attributeName, GLuint bufVBO, int variableSize) {
	GLuint location = shaderProgram->getAttribLocation(attributeName); 
	glBindBuffer(GL_ARRAY_BUFFER, bufVBO);  
	glEnableVertexAttribArray(location); 
	glVertexAttribPointer(location, variableSize, GL_FLOAT, GL_FALSE, 0, NULL); 
}

void setupVAO(Model *model) {
	glGenVertexArrays(1, &(model->vao));
	glBindVertexArray(model->vao);

	assignVBOtoAttribute("vertex", model->bufVertices, 4); 
	assignVBOtoAttribute("color", model->bufColors, 4);
	assignVBOtoAttribute("normal", model->bufNormals, 4); 
	assignVBOtoAttribute("texCoord",model->bufTexCoords,2);

	glBindVertexArray(0);
}

void displayFrame() {
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	matP=perspective(cameraAngle, (float)windowWidth/(float)windowHeight, 1.0f, 100.0f);

	matV=lookAt(vec3(cameraX,cameraY,cameraDistance),vec3(0.0f,1.0f,0.0f),vec3(0.0f,1.0f,0.0f)); 

	for (i = 0; i < 12; i++) drawObject(&piano[i]);
	for (i = 0; i < 88; i++) drawObject(&key[i]);
	for (i = 0; i < 88; i++) drawObject(&mechA[i]);
	for (i = 0; i < 88; i++) drawObject(&mechB[i]);
	glutSwapBuffers();
	
}

GLuint makeBuffer(void *data, int vertexCount, int vertexSize) {
	GLuint handle;
	
	glGenBuffers(1,&handle);
	glBindBuffer(GL_ARRAY_BUFFER,handle); 
	glBufferData(GL_ARRAY_BUFFER, vertexCount*vertexSize, data, GL_STATIC_DRAW);
	
	return handle;
}

void setupVBO(Model *model) {
	model->bufVertices = makeBuffer(model->v, model->vertexCount, sizeof(float)*4); 
	model->bufColors = makeBuffer(model->colors, model->vertexCount, sizeof(float)* 4);
	model->bufNormals = makeBuffer(model->vn, model->vertexCount, sizeof(float)* 4);
	model->bufTexCoords = makeBuffer(model->vt, model->vertexCount, sizeof(float)*4);
}

void nextFrame(void) {

	MIDIin->getMessage(&MIDImessage);
	
	if (MIDImessage.size() > 0){
		int nr = (int)MIDImessage[1] - 9;
		if ((int)MIDImessage[0] == 144){
			if (!key[nr].animationOn) {
				key[nr].animateDown = true;
				key[nr].animationOn = true;
			}
		}
		else if ((int)MIDImessage[0] == 128){
			key[nr].animateUp = true;
		}
		else if ((int)MIDImessage[0] == 176){
			if (sustain){
				for (i = 0; i < 88; i++) if (!key[i].animationOn) engine[i]->stopAllSounds();
			}
			sustain = !sustain;
			piano[5].animateDown = !piano[5].animateDown;
			piano[5].openCloseCover = true;
		}
	}
	
	glutPostRedisplay();
}

void specialKeyDown(int c, int x, int y) {
	switch (c) {
	case GLUT_KEY_LEFT:
		cameraX -= 0.05f;
		break;
	case GLUT_KEY_RIGHT:
		cameraX += 0.05f;
		break;
	case GLUT_KEY_UP:
		cameraDistance -= 0.05f;
		break;
	case GLUT_KEY_DOWN:
		cameraDistance += 0.05f;
		break;
	case GLUT_KEY_PAGE_UP:
		cameraY += 0.1f;
		break;
	case GLUT_KEY_PAGE_DOWN:
		cameraY -= 0.1f;
		break;
	case GLUT_KEY_F1:
		piano[0].visible = !piano[0].visible;
		break;
	case GLUT_KEY_F2:
		piano[1].visible = !piano[1].visible;
		break;
	case GLUT_KEY_F3:
		piano[2].visible = !piano[2].visible;
		piano[11].visible = !piano[11].visible;
		break;
	case GLUT_KEY_F4:
		piano[3].visible = !piano[3].visible;
		break;
	case GLUT_KEY_F5:
		piano[10].visible = !piano[10].visible;
		break;
	case GLUT_KEY_F6:
		piano[6].visible = !piano[6].visible;
		break;
	case GLUT_KEY_F7:
		piano[7].visible = !piano[7].visible;
		break;
	case GLUT_KEY_F8:
		piano[5].visible = !piano[5].visible;
		piano[8].visible = !piano[8].visible;
		piano[9].visible = !piano[9].visible;
		break;
	case GLUT_KEY_F9:
		piano[4].visible = !piano[4].visible;
		break;
	case GLUT_KEY_CTRL_L:
		if (sustain){
			for (i = 0; i < 88; i++) if (!key[i].animationOn) engine[i]->stopAllSounds();
		}
		sustain = !sustain;
		piano[5].animateDown = !piano[5].animateDown;
		piano[5].openCloseCover = true;
		break;
	case GLUT_KEY_ALT_L:
	case GLUT_KEY_CTRL_R:
		if (sustain){
			for (i = 0; i < 88; i++) if (!key[i].animationOn) engine[i]->stopAllSounds();
			piano[5].animateDown = true;
			piano[5].openCloseCover = true;
			piano[5].animationOn = true;
			
		}
		break;
	case GLUT_KEY_F10:
		cameraAngle += 1.0f;
		break;
	case GLUT_KEY_F11:
		cameraAngle -= 1.0f;
		break;
	case GLUT_KEY_INSERT:
		piano[1].animateDown = !piano[1].animateDown;
		piano[1].openCloseCover = true;
		break;
	}
}


void keyDown(unsigned char c, int x, int y){
	int klawisz;
	klawisz = klawisze.find_first_of(c, 0)+26;
	if (!key[klawisz].animationOn) {
		key[klawisz].animateDown = true;
		key[klawisz].animationOn = true;
	}
}

void keyUp(unsigned char c, int x, int y){
	int klawisz;
	klawisz = klawisze.find_first_of(c, 0)+26;
	key[klawisz].animateUp = true;
	switch (c){
	case 27:
		freeVAO();
		freeVBO();
		cleanShaders();
		exit(0);
		break;
	}
}

void changeSize(int w, int h) {
	glViewport(0,0,w,h);
	windowWidth=w;
	windowHeight=h;
}

void initGLUT(int *argc, char** argv) {
	glutInit(argc,argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH); 

	glutCreateWindow("PIANO PIANO");
	glutGameModeString("1920x1080:32@60");
	glutEnterGameMode();
	
	glutReshapeFunc(changeSize);
	glutDisplayFunc(displayFrame); 
	glutIdleFunc(nextFrame); 

	glutSpecialFunc(specialKeyDown);
	glutKeyboardFunc(keyDown);
	glutKeyboardUpFunc(keyUp);
}

void initGLEW() {
	GLenum err=glewInit();
	if (GLEW_OK!=err) {
		fprintf(stderr,"%s\n",glewGetErrorString(err));
		exit(1);
	}
	
}

void setupShaders() {
	shaderProgram = new ShaderProgram("vshader.txt",NULL,"fshader.txt");
	shaderProgram->use();
	glUniform4f(shaderProgram->getUniformLocation("lightPosition"), -3, 2, 3, 0);
	glUniform4f(shaderProgram->getUniformLocation("light2Position"), 2, 2, 1, 0);
}


GLuint readTexture(char* filename) {
	GLuint tex;
	TGAImg img;
	cout << filename << endl;
	glActiveTexture(GL_TEXTURE0);
	if (img.Load(filename)==IMG_OK) {
		glGenTextures(1,&tex); 
		glBindTexture(GL_TEXTURE_2D,tex); 
		glTexImage2D(GL_TEXTURE_2D,0,3,img.GetWidth(),img.GetHeight(),0,
		GL_RGB,GL_UNSIGNED_BYTE,img.GetImg()); 
	} else {
		printf("B³¹d przy wczytywaniu pliku: %s\n",filename);
	}
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,GL_REPEAT);
	return tex;
}

void initOpenGL() {
	setupShaders();
	for (i = 0; i < 12; i++) setupVBO(&piano[i]);
	for (i = 0; i < 88; i++) {
		setupVBO(&key[i]);
		setupVBO(&mechA[i]);
		setupVBO(&mechB[i]);
	}

	for (i = 0; i < 12; i++) setupVAO(&piano[i]);
	for (i = 0; i < 88; i++) {
		setupVAO(&key[i]);
		setupVAO(&mechA[i]);
		setupVAO(&mechB[i]);
	}
	glEnable(GL_DEPTH_TEST);

	texWood=readTexture("tex/base.tga");
	texBlack=readTexture("tex/black.tga");
	texBrass = readTexture("tex/brass.tga");
	texBrightWood = readTexture("tex/bright.tga");
	texCopper = readTexture("tex/copper.tga");
	texWhite = readTexture("tex/white.tga");
	texRed = readTexture("tex/red.tga");
}

void initModel(const char * path, Model *model){
	model->vertexCount = countElements(path) * 3;
	model->v = (float*)malloc(model->vertexCount * 4 * sizeof(float));
	model->vn = (float*)malloc(model->vertexCount * 4 * sizeof(float));
	model->vt = (float*)malloc(model->vertexCount * 4 * sizeof(float));
	loadOBJ(path, model->v, model->vt, model->vn);
}

void initModels(){
	initModel("obj/buda.obj",&piano[0]);
	initModel("obj/klawisznik.obj", &piano[1]);
	initModel("obj/klapa.obj", &piano[2]);
	initModel("obj/pokrywa.obj", &piano[3]);
	initModel("obj/deska.obj", &piano[4]);
	initModel("obj/pedalPrawy.obj", &piano[5]);
	initModel("obj/kolki.obj", &piano[6]);
	initModel("obj/kolka.obj", &piano[7]);
	initModel("obj/pedalLewy.obj", &piano[8]);
	initModel("obj/pedalSrodkowy.obj", &piano[9]);
	initModel("obj/struny.obj", &piano[10]);
	initModel("obj/czerwonyPasek.obj", &piano[11]);
	initModel("obj/key1.obj", &key[0]);
	initModel("obj/mecha1.obj",&mechA[0]);
	initModel("obj/mechb1.obj", &mechB[0]);

	for (i = 1; i < 86; i += 12) {
		initModel("obj/key2.obj", &key[i]);
		initModel("obj/mecha2.obj", &mechA[i]);
		initModel("obj/mechb2.obj", &mechB[i]);
		key[i].texPointer = &texBlack;
	}

	for (i = 2; i < 87; i += 12) {
		initModel("obj/key3.obj", &key[i]);
		initModel("obj/mecha3.obj", &mechA[i]);
		initModel("obj/mechb3.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 3; i < 88; i += 12) {
		if (i!=87) initModel("obj/key4.obj", &key[i]);
		initModel("obj/mecha4.obj", &mechA[i]);
		initModel("obj/mechb4.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 4; i < 87; i += 12) {
		initModel("obj/key5.obj", &key[i]);
		initModel("obj/mecha5.obj", &mechA[i]);
		initModel("obj/mechb5.obj", &mechB[i]);
		key[i].texPointer = &texBlack;
	}
	for (i = 5; i < 87; i += 12) {
		initModel("obj/key6.obj", &key[i]);
		initModel("obj/mecha6.obj", &mechA[i]);
		initModel("obj/mechb6.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 6; i < 87; i += 12){
		initModel("obj/key7.obj", &key[i]);
		initModel("obj/mecha7.obj", &mechA[i]);
		initModel("obj/mechb7.obj", &mechB[i]);
		key[i].texPointer = &texBlack;
	}
	for (i = 7; i < 87; i += 12) {
		initModel("obj/key8.obj", &key[i]);
		initModel("obj/mecha8.obj", &mechA[i]);
		initModel("obj/mechb8.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 8; i < 87; i += 12) {
		initModel("obj/key9.obj", &key[i]);
		initModel("obj/mecha9.obj", &mechA[i]);
		initModel("obj/mechb9.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 9; i < 87; i += 12) {
		initModel("obj/key10.obj", &key[i]);
		initModel("obj/mecha10.obj", &mechA[i]);
		initModel("obj/mechb10.obj", &mechB[i]);
		key[i].texPointer = &texBlack;
	}
	for (i = 10; i < 87; i += 12) {
		initModel("obj/key11.obj", &key[i]);
		initModel("obj/mecha11.obj", &mechA[i]);
		initModel("obj/mechb11.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	for (i = 11; i < 87; i += 12) {
		initModel("obj/key12.obj", &key[i]);
		initModel("obj/mecha12.obj", &mechA[i]);
		initModel("obj/mechb12.obj", &mechB[i]);
		key[i].texPointer = &texBlack;
	}
	for (i = 12; i < 87; i += 12) {
		initModel("obj/key13.obj", &key[i]);
		initModel("obj/mecha13.obj", &mechA[i]);
		initModel("obj/mechb13.obj", &mechB[i]);
		key[i].texPointer = &texWhite;
	}
	
	
	initModel("obj/keyLAST.obj", &key[87]);
	key[0].texPointer = &texWhite;

	for (i = 0; i < 88; i++){

		key[i].type = 1;
		mechA[i].type = 2;
		mechB[i].type = 3;
		mechA[i].texPointer = &texBrightWood;
		mechB[i].texPointer = &texBrightWood;
		key[i].nr = i;
		mechA[i].nr = i;
		mechB[i].nr = i;

		if (i>12 && i <= 24){
			key[i].offset = off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset = off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>24 && i <= 36){
			key[i].offset = 2*off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset = 2*off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 2*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>36 && i <= 48){
			key[i].offset = 3*off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset = 3*off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 3*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>48 && i <= 60){
			key[i].offset = 4 * off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset = 4*off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 4*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>60 && i <= 72){
			key[i].offset = 5 * off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset = 5*off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 5*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>72 && i <= 84){
			key[i].offset = 6 * off;
			key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			mechA[i].offset =6* off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 6*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
		if (i>84 && i <= 87){
			if (i <= 86){
				key[i].offset = 7 * off;
				key[i].modelMatrix = translate(mat4(1.0f), vec3(key[i].offset, 0, 0));
			}
			mechA[i].offset = 7*off;
			mechA[i].modelMatrix = translate(mat4(1.0f), vec3(mechA[i].offset, 0, 0));
			mechB[i].offset = 7*off;
			mechB[i].modelMatrix = translate(mat4(1.0f), vec3(mechB[i].offset, 0, 0));
		}
	}

	for (i = 0; i < 5; i++){
		piano[i].texPointer = &texWood;
	}
	for (i = 5; i < 10; i++){
		 piano[i].texPointer = &texBrass;
	}
	piano[10].texPointer = &texCopper;
	piano[11].texPointer = &texRed;

}

void loadSounds(){
	int i;
	for (i = 0; i < 88; i++) {
		engine[i] = createIrrKlangDevice();
		ostringstream ss;
		ss << i+21;
		string str = ss.str();
		if (i<100) str = "samples/mcg_mf_0" + str + ".wav"; else str = "samples/mcg_mf_" + str + ".wav";
		snd[i] = engine[i]->addSoundSourceFromFile(str.c_str());
	}
}

void initMIDI(){
	MIDIin->getPortCount();
	MIDIin->openPort(0);
	MIDIin->ignoreTypes(true, true, true);
	
}

int main(int argc, char** argv) {
	initMIDI();
	loadSounds();
	initModels();
	initGLUT(&argc,argv);
	initGLEW();
	initOpenGL();
	glutMainLoop();
	return 0;
}
