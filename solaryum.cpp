#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "Angel.h"
using namespace std;

const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumPlanet           = 10;
const int NumVertices         = 3 * NumTriangles * NumPlanet;


typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

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

enum { Xaxis = 0,
	Yaxis = 1,
	Zaxis = 2,
	NumAxes = 3 };
int Axis = Xaxis;
GLfloat Theta[NumAxes] = { -90, 0, 0 };

double zoomFactor = 0.03;
// Model-view and projection matrices uniform location
GLuint  CameraView, ModelView, Projection, Lighting;

bool lighting = false;
double sunTurn[NumPlanet];
double ownTurn[NumPlanet];
double inclination[NumPlanet];


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

	const vec3 displacement(distanceFromSun, 0, 0); //displacement for each cube to seperate.
	mat4 model_view;

	model_view =  Translate(displacement) * Scale(radius, radius, radius); // Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h
	modelView[rank] = model_view;

	FILE *fin;
	char tempstr[1024];
	fin = fopen(textureName, "rb");
	if (fin == NULL){
		cout << "Input file not found" << endl;
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

	imageGeneral = (GLubyte**) malloc(sizeof(GLubyte*) * NumPlanet);
	createPlanet(1.5,0,0, "sunmap.ppm");
	createPlanet(0.038,0.57+0.038+1,1, "mercurymap.ppm");
	createPlanet(0.095,1.08+0.57+0.038+1,2,"venusmap.ppm");
	createPlanet(0.1,1.08+0.57+0.038+1+1.50,3,"earthmap1k.ppm");
	createPlanet(0.053,2.28+1.08+0.57+0.038+1+1.50,4,"mars_1k_color.ppm");
	createPlanet(1.119,7.79+2.28+1.08+0.57+0.038+1+1.50,5,"jupitermap.ppm");
	createPlanet(0.940,14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,6,"saturn.ppm");
	createPlanet(0.404,28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,7,"uranusmap.ppm");
	createPlanet(0.388,45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,8,"neptunemap.ppm");
	createPlanet(0.018,59.10+45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,9,"plutomap1k.ppm");

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

	sunTurn[0] = 0; sunTurn[1] = 87.97; sunTurn[2] = 224.7; sunTurn[3] = 365.26; sunTurn[4] = 686.98; sunTurn[5] = 4331.98;
	sunTurn[6] = 10760.56; sunTurn[7] = 30707.41; sunTurn[8] = 60202.15; sunTurn[9] = 90803.64;

	ownTurn[0] = 28; ownTurn[1] = 58.65; ownTurn[2] = 243.01; ownTurn[3] = 1; ownTurn[4] = 1.0257; ownTurn[5] = 0.414;
	ownTurn[6] = 0.444; ownTurn[7] = 0.718; ownTurn[8] = 0.671; ownTurn[9] = 6.39;

	inclination[0] = 0; inclination[1] = 0; inclination[2] = 178; inclination[3] = 23.4; inclination[4] = 25; inclination[5] = 3.08;
	inclination[6] = 26.7; inclination[7] = 97.9; inclination[8] = 26.9; inclination[9] = 122.5;


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
	GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
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

	glClearColor( 0.0, 0.0, 0.0, 0.0 ); /* black background */
}

//----------------------------------------------------------------------------

void
display( void )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	mat4 mm[NumPlanet];

	//traverses the model view matrix to global rotate and zoom-in features.

	for (int k = 0; k < NumPlanet; k++) {
		mm[k] = Scale(zoomFactor,zoomFactor,zoomFactor) *

				RotateX(Theta[Xaxis] ) * RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) *    modelView[k] * RotateX(inclination[k] ); //* RotateY(inclination[k]); //global rotate

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
	switch (k)
	{
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
		modelView[rank] = RotateZ(-1000/sunTurn[rank])*modelView[rank] * RotateZ(1000/ownTurn[rank]);
	else
		modelView[rank] = modelView[rank] * RotateZ(1000/ownTurn[rank]);
	//Theta[Yaxis] += speed;


}

void
idle( void )
{

	for(int i = 0; i < NumPlanet;i++){
		rotatePlanet(i,i*0.05 + 0.01);

	}
	//Theta[Yaxis] += 0.05;

	/* for(int i = 0; i < NumPlanet;i++){
		 modelView[i] =
	 }*/

	glutPostRedisplay();
}


int
main( int argc, char **argv )
{

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize( 1200, 512 );
	glutCreateWindow( "Sphere" );
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
