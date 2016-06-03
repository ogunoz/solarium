#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "Angel.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumPlanet           = 13;
const int NumVertices         = 3 * NumTriangles * NumPlanet;


typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

float pi = 3.14159265;
GLfloat yaw    = -90.0f;	// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right (due to how Eular angles work) so we initially rotate a bit to the left.
GLfloat pitch  =  0.0f;

glm::vec3 cameraPos   = glm::vec3(0.0, -5.0,  5.0);
glm::vec3 cameraFront = glm::vec3(0.0, 1.0, -1.0);
glm::vec3 cameraUp    = glm::vec3(0.0, 1.0,  0.0);

void timer(int p);
void keyboard(unsigned char k, int x, int y);

point4 points[NumVertices];
point4 point[NumPlanet][3*NumTriangles];
vec3   normals[NumVertices];

mat4 modelView[NumPlanet];
GLubyte** imageGeneral; //*imageMercure;
int width[NumPlanet], height[NumPlanet];


int mouseX;
int mouseY;
int anim_count = 0, anim = 0, lastSelection = 1;
double changeUnit = 0;
double lastZoom;
double rotationSpeed = 100, lastSpeed = rotationSpeed, initSpeed = rotationSpeed;
bool projection = false;

enum { Xaxis = 0,
	Yaxis = 1,
	Zaxis = 2,
	NumAxes = 3 };

enum PLANETS{
	Space = 0, Sun, Mercury, Venus, Earth, Mars, Jupiter, Saturn, Uranus, Neptune, Pluto, SaturnRing, Moon
};
int Axis = Xaxis;
GLfloat Theta[NumAxes] = { -90, 0, 0 };

double zoomFactor = 0.03;
// Model-view and projection matrices uniform location
GLuint  CameraView, ModelView, Projection, Lighting, index, program;

bool lighting = false;
double sunTurn[NumPlanet];
double ownTurn[NumPlanet];
double inclination[NumPlanet];
double translate[NumPlanet];
double radiuses[NumPlanet];
double distances[NumPlanet];
const char* textureNames[NumPlanet];


//----------------------------------------------------------------------------

int Index = 0;
int localIndex = 0;

void
triangle( const point4& a, const point4& b, const point4& c, int rank )
{
	vec3  normal = normalize( cross(b - a, c - b) );

	normals[Index] = normal;  points[Index] = a;  Index++;
	normals[Index] = normal;  points[Index] = b;  Index++;
	normals[Index] = normal;  points[Index] = c;  Index++;

	point[rank][localIndex] = a; localIndex++;
	point[rank][localIndex] = b; localIndex++;
	point[rank][localIndex] = c; localIndex++;
}

//----------------------------------------------------------------------------

point4
unit( const point4& p )
{
	float len = p.x*p.x + p.y*p.y + p.z*p.z;

	point4 t;
	if ( len > DivideByZeroTolerance ) {
		t = p / sqrt(len);
		t.w = 1.0;
	}

	return t;
}

void
divide_triangle( const point4& a, const point4& b,
		const point4& c, int count, int rank )
{
	if ( count > 0 ) {
		point4 v1 = unit( a + b );
		point4 v2 = unit( a + c );
		point4 v3 = unit( b + c );
		divide_triangle(  a, v1, v2, count - 1, rank );
		divide_triangle(  c, v2, v3, count - 1, rank );
		divide_triangle(  b, v3, v1, count - 1, rank );
		divide_triangle( v1, v3, v2, count - 1, rank );
	}
	else {
		triangle( a, b, c, rank );
	}
}

void
tetrahedron( int count, int rank )
{
	point4 v[4] = {
			vec4( 0.0, 0.0, 1.0, 1.0 ),
			vec4( 0.0, 0.942809, -0.333333, 1.0 ),
			vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
			vec4( 0.816497, -0.471405, -0.333333, 1.0 )
	};

	divide_triangle( v[0], v[1], v[2], count, rank );
	divide_triangle( v[3], v[2], v[1], count, rank );
	divide_triangle( v[0], v[3], v[1], count, rank );
	divide_triangle( v[0], v[2], v[3], count, rank );

}

void createPlanet(double radius, double distanceFromSun, int rank, const char* textureName){
	localIndex = 0;
	tetrahedron( NumTimesToSubdivide, rank );

	translate[rank] = distanceFromSun;

	vec3 displacement(distanceFromSun, 0, 0); //displacement for each cube to seperate.
	mat4 model_view;

	if (rank == Space)
		displacement = vec3(200, 2500, distanceFromSun);

	mat4 scale;
	if(rank == SaturnRing)
		scale = Scale(radius*1.5, radius*1.5, radius*0.15);
	else
		scale = Scale(radius, radius, radius);

	model_view =   RotateX(inclination[rank] ) * Translate(displacement) * scale; // Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h

	modelView[rank] = model_view;

	FILE *fin;
	char tempstr[1024];
	fin = fopen(textureName, "rb");
	if (fin == NULL){
		cout << "Input file not found" << endl;
		cout << textureName << endl;
		return;
	}
	// First two lines are unnecessary
	int maxLevel;

	//fgets(tempstr, 1024, fin);
	int wi, he;

	fgets(tempstr, 1024, fin);
	fgets(tempstr, 1024, fin);
	sscanf(tempstr, "%d %d", &wi, &he);
	fgets(tempstr, 1024, fin);
	sscanf(tempstr, "%d", &maxLevel);

	imageGeneral[rank] = (GLubyte*)malloc(sizeof(GLubyte) * wi * he * 3);

	GLubyte image[wi][he][3];

	char aByte;
	for (int i = 0;i<wi;i++){
		for(int j = 0; j <he;j++){
			for (int c = 0; c < 3; c++){
				aByte =  fgetc(fin);
				if(rank == Moon && c == 0)
					image[wi-i-1][j][c] = aByte-0.00001; // moon r g b ayný renkte olduðu için böyle yaptýk
				else
					image[wi-i-1][j][c] = aByte;
			}
		}
	}
	fclose(fin);
	int i,j,c;
	for (i = 0;i<wi;i++){
		for(j = 0; j <he;j++){
			for (c = 0; c < 3; c++){
				imageGeneral[rank][3*he*i+3*j+c] = image[i][j][c];
			}
		}
	}

	width[rank] = wi;
	height[rank] = he;
}


//----------------------------------------------------------------------------

// OpenGL initialization
void
init(){

	distances[0] = -4000; distances[1] = 0; distances[2] = 0.57+0.038+1; distances[3] = 1.08+0.57+0.038+1;
	distances[4] = 1.08+0.57+0.038+1+1.50; distances[5] = 2.28+1.08+0.57+0.038+1+1.50;
	distances[6] = 7.79+2.28+1.08+0.57+0.038+1+1.50; distances[7] = 14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[8] = 28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1; distances[9] = 45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[10] = 59.10+45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50; distances[11] = distances[7]; distances[12] = 1.08+0.57+0.038+1+1.50 + 0.1 + 0.2;

	sunTurn[0] = 0; sunTurn[1] = 0; sunTurn[2] = 87.97; sunTurn[3] = 224.7; sunTurn[4] = 365.26; sunTurn[5] = 686.98; sunTurn[6] = 4331.98;
	sunTurn[7] = 10760.56; sunTurn[8] = 30707.41; sunTurn[9] = 60202.15; sunTurn[10] = 90803.64; sunTurn[11] = sunTurn[7]; sunTurn[12] = sunTurn[4];

	ownTurn[0] = 0; ownTurn[1] = 28; ownTurn[2] = 58.65; ownTurn[3] = 243.01; ownTurn[4] = 1; ownTurn[5] = 1.0257; ownTurn[6] = 0.414;
	ownTurn[7] = 0.444; ownTurn[8] = 0.718; ownTurn[9] = 0.671; ownTurn[10] = 6.39; ownTurn[11] = ownTurn[7]; ownTurn[12] = 27.32;

	inclination[0] = 0; inclination[1] = 0; inclination[2] = 0; inclination[3] = 178; inclination[4] = 23.4; inclination[5] = 25; inclination[6] = 3.08;
	inclination[7] = 26.7; inclination[8] = 97.9; inclination[9] = 26.9; inclination[10] = 122.5; inclination[11] = inclination[7]; inclination[12] = 0;

	radiuses[Space] = 3000.0; radiuses[Sun] = 1.5; radiuses[Mercury] = 0.038; radiuses[Venus] = 0.095; radiuses[Earth] = 0.1; radiuses[Moon] = 0.027;
	radiuses[Mars] = 0.053; radiuses[Jupiter] = 1.119; radiuses[Saturn] = 0.940; radiuses[Uranus] = 0.404; radiuses[Neptune] = 0.388; radiuses[Pluto] = 0.018;
	radiuses[SaturnRing] = 0.940;

	textureNames[Space] = "space.ppm"; textureNames[Sun] = "sunmap.ppm"; textureNames[Mercury] = "mercurymap.ppm"; textureNames[Venus] = "venusmap.ppm";
	textureNames[Earth] = "earthmap1k.ppm"; textureNames[Moon] = "mooooon.ppm"; textureNames[Mars] = "mars_1k_color.ppm"; textureNames[Jupiter] = "jupitermap.ppm";
	textureNames[Saturn] = "saturn.ppm"; textureNames[Uranus] = "uranusmap.ppm"; textureNames[Neptune] = "neptunemap.ppm"; textureNames[Pluto] = "plutomap1k.ppm";
	textureNames[SaturnRing] = "saturnring.ppm";


	imageGeneral = (GLubyte**) malloc(sizeof(GLubyte*) * NumPlanet);
	for(int i = 0; i < NumPlanet; i++)
		createPlanet(radiuses[i], distances[i], i, textureNames[i]);

	// Create a vertex array object
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	GLintptr offset = 0;

	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals) + sizeof(point), NULL, GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(points), points );
	offset+= sizeof(points);
	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(normals), normals );
	offset += sizeof(normals);

	for(int i = 0; i < NumPlanet; i++){
		glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(point[i]), point[i] );
		offset += sizeof(point[i]);
	}


	// Load shaders and use the resulting shader program
	program = InitShader( "vshader.glsl", "fshader.glsl" );
	glUseProgram( program );

	GLuint vPosition = glGetAttribLocation( program, "vPosition" );
	glEnableVertexAttribArray( vPosition );
	glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	GLuint vNormal = glGetAttribLocation( program, "vNormal" );
	glEnableVertexAttribArray( vNormal );
	glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
			BUFFER_OFFSET(sizeof(points)) );

	GLuint vTexCoord = glGetAttribLocation( program, "vTexCoord");
	glEnableVertexAttribArray( vTexCoord );
	glVertexAttribPointer( vTexCoord, 4, GL_FLOAT, GL_FALSE, 0,
			BUFFER_OFFSET(sizeof(points) + sizeof(normals)) );


	// Initialize shader lighting parameters
	point4 light_position( 0.0, 0.0, 0.0, 1.0 );
	color4 light_ambient( 0.5, 0.5, 0.5, 1.0 );
	color4 light_diffuse( 1.0, 1.0, 1.0, 1.0 );
	color4 light_specular( 0.5, 0.5, 0.5, 1.0 );

	color4 material_ambient = color4( 0.2, 0.2, 0.2, 1.0 );
	color4 material_diffuse = color4( 1.0, 1.0, 1.0, 1.0 );
	color4 material_specular = color4(1.0, 1.0, 1.0, 1.0 );
	//color4 material_specular = color4(1.0, 1.0, 1.0, 1.0 );
	float material_shininess = 1000;

	color4 ambient_product = light_ambient * material_ambient;
	color4 diffuse_product = light_diffuse * material_diffuse;
	color4 specular_product = light_specular * material_specular;

	glUniform4fv( glGetUniformLocation(program, "AmbientProduct"),
			1, ambient_product );
	glUniform4fv( glGetUniformLocation(program, "DiffuseProduct"),
			1, diffuse_product );
	glUniform4fv( glGetUniformLocation(program, "SpecularProduct"),
			1, specular_product );
	glUniform4fv( glGetUniformLocation(program, "LightPosition"),
			1, light_position );

	glUniform1f( glGetUniformLocation(program, "Shininess"),
			material_shininess );

	Lighting = glGetUniformLocation(program, "lighting");
	glUniform1i( Lighting, lighting );

	// Retrieve transformation uniform variable locations
	ModelView = glGetUniformLocation( program, "ModelView" );
	Projection = glGetUniformLocation( program, "Projection" );
	CameraView = glGetUniformLocation( program, "CameraView" );

	glEnable( GL_DEPTH_TEST );
	glEnable(GL_CULL_FACE); //to discard invisible faces from rendering
	glEnable(GL_STENCIL_TEST);

	glClearColor( 0.0, 0.0, 0.0, 1.0 ); /* black background */
}

//----------------------------------------------------------------------------

void
display( void )
{
	glClearStencil(0);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	mat4 mm[NumPlanet];

	glm::vec3 target = cameraPos + cameraFront;
	glm::mat4 cameraView;


	if (projection)
		cameraView = glm::mat4();
	else
		cameraView = glm::lookAt(cameraPos, target, cameraUp);

	glUniformMatrix4fv(CameraView, 1, GL_FALSE, glm::value_ptr(cameraView));
	//traverses the model view matrix to global rotate and zoom-in features.

	double dist = 0;
	for(int i = 1; i <= lastSelection; i++){
		dist+= distances[i];
	}
	point4 light_position( -dist*zoomFactor, 0.0, 0.0, 1.0 );

	glUniform4fv( glGetUniformLocation(program, "LightPosition"), 1, light_position );



	for (int k = 0; k < NumPlanet; k++) {
		glUniform1i( glGetUniformLocation(program, "isSun"), 0 );
		if(k == Sun)
			glUniform1i( glGetUniformLocation(program, "isSun"), 1 );

		if (k != Space){
			glStencilFunc(GL_ALWAYS, k,0);
			if(projection)
				mm[k] = Scale(zoomFactor, zoomFactor, zoomFactor) * RotateX(Theta[Xaxis]) * RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) * modelView[k]; //* RotateY(inclination[k]); //global rotate
			else
				mm[k] = modelView[k]; //* RotateY(inclination[k]); //global rotate
		}
		else{
			mm[k] = Scale(0.03, 0.03, 0.03) *modelView[k]; //* RotateY(inclination[k]); //global rotate
		}
		glUniformMatrix4fv(ModelView, 1, GL_TRUE, mm[k]);

		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width[k], height[k] , 0, GL_RGB, GL_UNSIGNED_BYTE, imageGeneral[k]);

		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		glActiveTexture( GL_TEXTURE0);

		//  glUniformMatrix4fv(CameraView, 1, GL_TRUE, view);

		glPolygonMode(GL_FRONT, GL_FILL);
		glDrawArrays(GL_TRIANGLES, k*NumVertices / NumPlanet, NumVertices / NumPlanet);
	}

	//glFlush();
	glutSwapBuffers();
}

//----------------------------------------------------------------------------


void
reshape( int w, int h )
{

	if(projection){

		glViewport( 0, 0, w, h );

		mat4  projection;
		if (w <= h)
			projection = Ortho(-1.0, 1.0, -1.0 * (GLfloat) h / (GLfloat) w,
					1.0 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
		else  projection = Ortho(-1.0* (GLfloat) w / (GLfloat) h, 1.0 *
				(GLfloat) w / (GLfloat) h, -1.0, 1.0, -10.0, 10.0);
		glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	}
	else{
		glViewport( 0, 0, w, h );


		GLfloat aspect;
		if (w < h){
			aspect = GLfloat(h)/w;
		}
		else{
			aspect = GLfloat(w)/h;
		}



		mat4  projection;
		projection = Perspective(45.0, aspect, 1.0f, 100.0f);
		glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
		glutPostRedisplay();

	}

}

//----------------------------------------------------------------------------

//Mouse callback function.
void mouse( int button, int state, int x, int y )
{
	GLfloat cameraSpeed = 0.1f;
	if ( state == GLUT_UP ) {
		switch( button ) {
		case 3:
			if(projection)
				zoomFactor *= 1.1;
			else{
				cameraPos += cameraSpeed * cameraFront;
			}
			break; //Scroll-up
		case 4:
			if(projection)
				zoomFactor *= 0.9;
			else{
				cameraPos -= cameraSpeed * cameraFront;

			}
			break; //Scroll-down
		}

		glutPostRedisplay();
	}
	else if ( state == GLUT_DOWN && button == GLUT_LEFT_BUTTON) {
		y = glutGet( GLUT_WINDOW_HEIGHT ) - y;

		unsigned char pixel[4];
		glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
		//GLuint index;
		index = 0;


		if(pixel[0] != pixel[1] || pixel[1] != pixel[2]){
			glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);
			changeUnit = -translate[index-1];

			if(index == SaturnRing)
				index = Saturn;
			int indexReal = index;



			for(int i = 1; i < NumPlanet; i++){
				//	modelView[i] =  modelView[i] * Translate(distances[index],0,0);
			}



			if (index != lastSelection){
				keyboard('*',0,0);
				index = indexReal;
				lastSelection = index;
				zoomFactor = 0.03;
				changeUnit = -translate[index];
				anim_count = anim = 40;
				timer(10);
				lastZoom = zoomFactor;
			}

			glutPostRedisplay();
		}



		/*	if(pixel[0] != 0 || pixel[1] != 0 || pixel[2] != 0){
			glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);
			changeUnit = -translate[index-1];

			int indexReal = index;
			if (index != lastSelection){
				keyboard('*',0,0);
				index = indexReal;
				lastSelection = index;
				zoomFactor = 0.03;
				changeUnit = -translate[index];
				anim_count = anim = 40;
				timer(10);
				lastZoom = zoomFactor;
			}

			glutPostRedisplay();
		}*/

	}

}
void specialKeyboard(int k, int x, int y)

{
	if(projection){
		switch (k) {
		case GLUT_KEY_LEFT:
			Theta[Zaxis] += 3; // increase Z rotation by 3 degrees
			break;
		case GLUT_KEY_RIGHT:
			Theta[Zaxis] -= 3; // decrease Z rotation by 3 degrees
			break;
		case GLUT_KEY_UP:
			Theta[Xaxis] += 3; // increase X rotation by 3 degrees
			break;
		case GLUT_KEY_DOWN:
			Theta[Xaxis] -= 3; // decrease X rotation by 3 degrees
			break;

		}
		glutPostRedisplay();
	}
}

void keyboard(unsigned char k, int x, int y)
{
	GLfloat cameraSpeed = 2;
	vec3 displacement;
	mat4 model_view;

	switch (k){
	int in;
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '0':

		in = k - '0';
		if (in+1 != lastSelection){
			keyboard('*',0,0);
			lastSelection = in+1;
			changeUnit = -translate[in+1];
			index = in+1;
			anim_count = anim = 40;
			timer(10);
			lastZoom = zoomFactor;

		}

		break;
	case '*':
		changeUnit = -translate[lastSelection];
		index = 1;

		for(int i = 1; i < NumPlanet; i++){
			modelView[i] = Translate(changeUnit, 0, 0) * modelView[i];
			translate[i] = changeUnit + translate[i];
		}
		zoomFactor = 0.03;
		lastZoom = zoomFactor;
		//lastSelection = -1;

		break;
		//
	case 'n':
		projection = !projection;
		reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

		if(projection){
			displacement = vec3(0, 0, -450);
			model_view =   RotateX(inclination[Space] ) * Translate(displacement) * Scale(200, 200, 200);
		}
		else{
			displacement = vec3(200, 2500, -4000);
			model_view =   RotateX(inclination[Space] ) * Translate(displacement) * Scale(3000, 3000, 3000);
		}
		// Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h

		modelView[0] = model_view;


		break;
	case 'A':
	case 'a':   //Initialize the object to default angle.
		if(!projection){
			cameraPos -= normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
			modelView[0] = modelView[0] * Translate(0.0111111 * -cameraSpeed,0,0);
		}
		break;
	case 'S':
	case 's': //Initialize the object to default zoom value.
		if(!projection){
			cameraPos.y -= cameraSpeed;
			modelView[0] = modelView[0] * Translate(0,0.0111111 * -cameraSpeed,0);
		}
		break;
	case 'W':
	case 'w': //Initialize the object to default zoom value.
		if(!projection){
			cameraPos.y +=  cameraSpeed;
			modelView[0] = modelView[0] * Translate(0,0.0111111 * cameraSpeed,0);
		}

		break;
	case 'D':
	case 'd': //Initialize the object to default zoom value.
		if(!projection){
			cameraPos += normalize(cross(cameraFront, cameraUp)) * cameraSpeed;
			modelView[0] = modelView[0] * Translate(0.0111111 * cameraSpeed,0,0);
		}
		break;
	case 'L':
	case 'l':
		if (lighting == true) lighting = false;
		else lighting = true;
		glUniform1i( Lighting, lighting );
		break;

	case '+':
		rotationSpeed += 25;
		lastSpeed = rotationSpeed;
		break;
	case '-':
		if(rotationSpeed >= 25){
			rotationSpeed -= 25;
			lastSpeed = rotationSpeed;
		}
		break;

	case 'P':
	case 'p':
		if(rotationSpeed == 0)
			rotationSpeed = lastSpeed;
		else
			rotationSpeed = 0;
		break;


	case 'Q':
	case 'q':
		cout << "Program is terminated." << endl;
		exit(0);
		break;

	}
	glutPostRedisplay();		// redraw the image now
}

void rotatePlanet(int rank){
	if (rank != Sun && lastSelection == Sun){
		modelView[rank] =  RotateZ(-rotationSpeed/sunTurn[rank])*modelView[rank] * RotateZ(rotationSpeed/ownTurn[rank]);

	}
	else
		modelView[rank] = modelView[rank] * RotateZ(rotationSpeed/ownTurn[rank]);
	//Theta[Yaxis] += speed;
}

void
idle( void ){

	for(int i = Sun; i < NumPlanet;i++)
		//rotatePlanet(i);

	glutPostRedisplay();
}

void timer( int p ){
	if(anim_count > 0){
		anim_count--;

		for(int i = Sun; i < NumPlanet; i++){
			modelView[i] = Translate((changeUnit/anim), 0, 0) * modelView[i];
			translate[i] = (changeUnit/anim) + translate[i];
		}
		if (1 / radiuses[index-1] > lastZoom){
			zoomFactor += (1 / radiuses[index] - lastZoom) / anim;
		}
		else{
			zoomFactor -= (lastZoom - 1 / radiuses[index]) / anim;
		}
		glutPostRedisplay();
		glutTimerFunc(10,timer,p);

	}
	//	zoomFactor = 1/radiuses[index-1];
}



int
main( int argc, char **argv )
{

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize( 1600, 600 );
	glutCreateWindow( "Solar System" );
	glewExperimental = GL_TRUE;
	glewInit();
	init();

	glutMouseFunc( mouse );// set mouse callback function for mouse
	glutSpecialFunc(specialKeyboard); // set mouse callback function for mouse
	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );
	glutIdleFunc( idle );

	glutMainLoop();
	for(int i; i < NumPlanet; i++){
		free(imageGeneral[i]);
	}
	free(imageGeneral);
	return 0;
}
