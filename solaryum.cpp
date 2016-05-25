#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "Angel.h"
using namespace std;

const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumPlanet           = 11;
const int NumVertices         = 3 * NumTriangles * NumPlanet;


typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

void timer(int p);
void keyboard(unsigned char k, int x, int y);

point4 points[NumVertices];
point4 point[NumPlanet][3*NumTriangles];
vec3   normals[NumVertices];

mat4 modelView[NumPlanet];
GLubyte** imageGeneral; //*imageMercure;
int width[NumPlanet], height[NumPlanet];

vec3 cameraPos   = vec3(0.0f, 0.0f,  0.0f);
vec3 cameraFront = vec3(0.0f, 0.0f,   0.0f);
vec3 cameraUp    = vec3(0.0f, 0.0f,  0.0f);

int mouseX;
int mouseY;
int anim_count = 0, anim = 0, lastSelection = -1;
double changeUnit = 0;
double lastZoom;

enum { Xaxis = 0,
	Yaxis = 1,
	Zaxis = 2,
	NumAxes = 3 };
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
	radiuses[rank] = radius;

	vec3 displacement(distanceFromSun, 0, 0); //displacement for each cube to seperate.
	mat4 model_view;

	if (rank == 0){
		displacement = vec3(0, 0, distanceFromSun);
	}

	model_view =  Translate(displacement) * Scale(radius, radius, radius); // Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h
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
init()
{


	distances[0] = -450; distances[1] = 0; distances[2] = 0.57+0.038+1; distances[3] = 1.08+0.57+0.038+1;
	distances[4] = 1.08+0.57+0.038+1+1.50; distances[5] = 2.28+1.08+0.57+0.038+1+1.50;
	distances[6] = 7.79+2.28+1.08+0.57+0.038+1+1.50; distances[7] = 14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[8] = 28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1; distances[9] = 45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[10] = 59.10+45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;

	imageGeneral = (GLubyte**) malloc(sizeof(GLubyte*) * NumPlanet);
	createPlanet(200.0, distances[0], 0, "space.ppm");
	createPlanet(1.5,  distances[1],1, "sunmap.ppm");
	createPlanet(0.038,distances[2],2, "mercurymap.ppm");
	createPlanet(0.095,distances[3],3,"venusmap.ppm");
	createPlanet(0.1,distances[4],4,"earthmap1k.ppm");
	createPlanet(0.053,distances[5],5,"mars_1k_color.ppm");
	createPlanet(1.119,distances[6],6,"jupitermap.ppm");
	createPlanet(0.940,distances[7],7,"saturn.ppm");
	createPlanet(0.404,distances[8],8,"uranusmap.ppm");
	createPlanet(0.388,distances[9],9,"neptunemap.ppm");
	createPlanet(0.018,distances[10],10,"plutomap1k.ppm");

	/*GUNES		KENDI
	 * 0(GALAKTIK MERKEZIN ..)	28
	 * 87.97 - 58.65
	 * 224.7 - 243.01
	 * 365.26 - 1
	 * 686.98 - 1.0257
	 * 4331.98 - 0.414
	 * 10760.56 - 0.444
	 * 30707.41 - 0.718
	 * 60202.15 - 0.671
	 * 90803.64 - 6.39
	 */

	sunTurn[0] = 0; sunTurn[1] = 0; sunTurn[2] = 87.97; sunTurn[3] = 224.7; sunTurn[4] = 365.26; sunTurn[5] = 686.98; sunTurn[6] = 4331.98;
	sunTurn[7] = 10760.56; sunTurn[8] = 30707.41; sunTurn[9] = 60202.15; sunTurn[10] = 90803.64;

	ownTurn[0] = 0; ownTurn[1] = 28; ownTurn[2] = 58.65; ownTurn[3] = 243.01; ownTurn[4] = 1; ownTurn[5] = 1.0257; ownTurn[6] = 0.414;
	ownTurn[7] = 0.444; ownTurn[8] = 0.718; ownTurn[9] = 0.671; ownTurn[10] = 6.39;

	inclination[0] = 0; inclination[1] = 0; inclination[2] = 0; inclination[3] = 178; inclination[4] = 23.4; inclination[5] = 25; inclination[6] = 3.08;
	inclination[7] = 26.7; inclination[8] = 97.9; inclination[9] = 26.9; inclination[10] = 122.5;


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


	/*color4 material_ambient( 1.0, 0.0, 1.0, 1.0 );
	color4 material_diffuse( 1.0, 1.0, 0.0, 1.0 );
	color4 material_specular( 1.0, 0.0, 1.0, 1.0 );


	// Material properties
Material g_SunMaterial( color4(0,0,0,1), color4(1,1,1,1), color4(1,1,1,1) );
Material g_EarthMaterial( color4( 0.2, 0.2, 0.2, 1.0), color4( 1, 1, 1, 1), color4( 1, 1, 1, 1), color4(0, 0, 0, 1), 50 );
Material g_MoonMaterial( color4( 0.1, 0.1, 0.1, 1.0), color4( 1, 1, 1, 1), color4( 0.2, 0.2, 0.2, 1), color4(0, 0, 0, 1), 10 );

	float  material_shininess = 50.0;*/
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
	//    CameraView = glGetUniformLocation( program, "CameraView" );

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

	//traverses the model view matrix to global rotate and zoom-in feature
	cout << lastSelection << endl;

	//cout << translate[1] << endl;
	double dist = 0;
	for(int i = 1; i <= lastSelection; i++){
		dist+= distances[i];
	}
	point4 light_position( -dist*zoomFactor, 0.0, 0.0, 1.0 );


	cout << light_position.x << endl;
	//cout << translate[1] + radiuses[1] << endl;
	glUniform4fv( glGetUniformLocation(program, "LightPosition"), 1, light_position );



	for (int k = 0; k < NumPlanet; k++) {
		if (k != 0){
			glStencilFunc(GL_ALWAYS, k,0);
			mm[k] = Scale(zoomFactor,zoomFactor,zoomFactor) *
					RotateX(Theta[Xaxis] ) * RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) *    modelView[k] * RotateX(inclination[k] ); //* RotateY(inclination[k]); //global rotate
		}
		else{
			mm[k] = Scale(0.03,0.03,0.03) * RotateX(0 ) * RotateY(0) * RotateZ(0) *

					modelView[k] * RotateX(inclination[k] ); //* RotateY(inclination[k]); //global rotate
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

	glViewport( 0, 0, w, h );

	mat4  projection;
	if (w <= h)
		projection = Ortho(-1.0, 1.0, -1.0 * (GLfloat) h / (GLfloat) w,
				1.0 * (GLfloat) h / (GLfloat) w, -10.0, 10.0);
	else  projection = Ortho(-1.0* (GLfloat) w / (GLfloat) h, 1.0 *
			(GLfloat) w / (GLfloat) h, -1.0, 1.0, -10.0, 10.0);
	glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
}

//----------------------------------------------------------------------------

//Mouse callback function.
void mouse( int button, int state, int x, int y )
{

	if ( state == GLUT_UP ) {
		switch( button ) {
		case 3:    zoomFactor *= 1.1;  break; //Scroll-up
		case 4:    zoomFactor *= 0.9;  break; //Scroll-down
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

void keyboard(unsigned char k, int x, int y)
{
	//GLfloat cameraSpeed = 0.01f;
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
		// case 'a':   //Initialize the object to default angle.
		//   cameraPos -= normalize(cross(cameraFront, cameraUp)) * cameraSpeed;      break;
		// case 's': //Initialize the object to default zoom value.
		//         cameraPos -= cameraSpeed * cameraFront;
		// break;
		// case 'w': //Initialize the object to default zoom value.
		//   cameraPos += cameraSpeed * cameraFront;      break;
		//   case 'd': //Initialize the object to default zoom value.
		//   cameraPos += normalize(cross(cameraFront, cameraUp)) * cameraSpeed; break;
		//
	case 'l':
		if (lighting == true) lighting = false;
		else lighting = true;
		glUniform1i( Lighting, lighting );
		glutPostRedisplay();
		break;
	case 'Q':
	case 'q':
		cout << "Program is terminated." << endl;
		exit(0);
		break;

	}
	glutPostRedisplay();		// redraw the image now
}

void rotatePlanet(int rank, double speed){
	if (sunTurn[rank] != 0)
		modelView[rank] = RotateZ(-100/sunTurn[rank])*modelView[rank] * RotateZ(100/ownTurn[rank]);
	else
		modelView[rank] = modelView[rank] * RotateZ(100/ownTurn[rank]);
	//Theta[Yaxis] += speed;
}

void
idle( void ){

	for(int i = 1; i < NumPlanet;i++){
		//	rotatePlanet(i,i*0.05 + 0.01);
	}
	//Theta[Yaxis] += 0.05;

	/* for(int i = 0; i < NumPlanet;i++){
		 modelView[i] =
	 }*/

	glutPostRedisplay();
}

void timer( int p ){
	if(anim_count > 0){
		anim_count--;

		for(int i = 1; i < NumPlanet; i++){
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
