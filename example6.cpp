// fragment shading of sphere model

#include "Angel.h"

const int NumTimesToSubdivide = 5;
const int NumTriangles        = 4096;  // (4 faces)^(NumTimesToSubdivide + 1)
const int NumPlanet           = 10;
const int NumVertices         = 3 * NumTriangles * NumPlanet;


typedef Angel::vec4 point4;
typedef Angel::vec4 color4;

point4 points[NumVertices];
vec3   normals[NumVertices];

mat4 modelView[NumPlanet];

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
  GLfloat Theta[NumAxes] = { 15, 30.0, 0 };

double zoomFactor = 1;
// Model-view and projection matrices uniform location
GLuint  CameraView, ModelView, Projection;

//----------------------------------------------------------------------------

int Index = 0;

void
triangle( const point4& a, const point4& b, const point4& c )
{
    vec3  normal = normalize( cross(b - a, c - b) );

    normals[Index] = normal;  points[Index] = a;  Index++;
    normals[Index] = normal;  points[Index] = b;  Index++;
    normals[Index] = normal;  points[Index] = c;  Index++;
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
		 const point4& c, int count )
{
    if ( count > 0 ) {
        point4 v1 = unit( a + b );
        point4 v2 = unit( a + c );
        point4 v3 = unit( b + c );
        divide_triangle(  a, v1, v2, count - 1 );
        divide_triangle(  c, v2, v3, count - 1 );
        divide_triangle(  b, v3, v1, count - 1 );
        divide_triangle( v1, v3, v2, count - 1 );
    }
    else {
        triangle( a, b, c );
    }
}

void
tetrahedron( int count )
{
    point4 v[4] = {
	vec4( 0.0, 0.0, 1.0, 1.0 ),
	vec4( 0.0, 0.942809, -0.333333, 1.0 ),
	vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
	vec4( 0.816497, -0.471405, -0.333333, 1.0 )
    };

    divide_triangle( v[0], v[1], v[2], count );
    divide_triangle( v[3], v[2], v[1], count );
    divide_triangle( v[0], v[3], v[1], count );
    divide_triangle( v[0], v[2], v[3], count );
}

void createPlanet(double radius, double distanceFromSun, int rank){

  tetrahedron( NumTimesToSubdivide );
  const vec3 displacement(distanceFromSun, 0, 0); //displacement for each cube to seperate.
  mat4 model_view;

  model_view =  Translate(displacement) *Scale(radius, radius, radius) ; // Scale(), Translate(), RotateX(), RotateY(), RotateZ(): user-defined functions in mat.h
  modelView[rank] = model_view;

}


//----------------------------------------------------------------------------

// OpenGL initialization
void
init()
{
    createPlanet(2,0,0);
    createPlanet(0.038,0.57+0.038+1,1);
    createPlanet(0.095,1.08+0.57+0.038+1,2);
    createPlanet(0.1,1.08+0.57+0.038+1+1.50,3);
    createPlanet(0.053,2.28+1.08+0.57+0.038+1+1.50,4);
    createPlanet(1.119,7.79+2.28+1.08+0.57+0.038+1+1.50,5);
    createPlanet(0.940,14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,6);
    createPlanet(0.404,28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,7);
    createPlanet(0.388,45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,8);
    createPlanet(0.018,59.10+45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50,9);




    // Create a vertex array object
    GLuint vao;
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    // Create and initialize a buffer object
    GLuint buffer;
    glGenBuffers( 1, &buffer );
    glBindBuffer( GL_ARRAY_BUFFER, buffer );
    glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals),
		  NULL, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0, sizeof(points), points );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof(points),
		     sizeof(normals), normals );

    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader56.glsl", "fshader56.glsl" );
    glUseProgram( program );

    // set up vertex arrays
    GLuint vPosition = glGetAttribLocation( program, "vPosition" );
    glEnableVertexAttribArray( vPosition );
    glVertexAttribPointer( vPosition, 4, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(0) );

    GLuint vNormal = glGetAttribLocation( program, "vNormal" );
    glEnableVertexAttribArray( vNormal );
    glVertexAttribPointer( vNormal, 3, GL_FLOAT, GL_FALSE, 0,
			   BUFFER_OFFSET(sizeof(points)) );

    // Initialize shader lighting parameters
    point4 light_position( 0.0, 0.0, 2.0, 0.0 );
    color4 light_ambient( 0.2, 0.2, 0.2, 1.0 );
    color4 light_diffuse( 1.0, 1.0, 1.0, 1.0 );
    color4 light_specular( 1.0, 1.0, 1.0, 1.0 );

    color4 material_ambient( 1.0, 0.0, 1.0, 1.0 );
    color4 material_diffuse( 1.0, 0.8, 0.0, 1.0 );
    color4 material_specular( 1.0, 0.0, 1.0, 1.0 );
    float  material_shininess = 5.0;

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

    // Retrieve transformation uniform variable locations
    ModelView = glGetUniformLocation( program, "ModelView" );
    Projection = glGetUniformLocation( program, "Projection" );
//    CameraView = glGetUniformLocation( program, "CameraView" );






    glEnable( GL_DEPTH_TEST );

    glClearColor( 0.0, 0.0, 0.0, 0.0 ); /* white background */
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

          RotateX(Theta[Xaxis]) * RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) *    modelView[k]; //global rotate

          glUniformMatrix4fv(ModelView, 1, GL_TRUE, mm[k]);

        //  mat4 view = LookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        //  glUniformMatrix4fv(CameraView, 1, GL_TRUE, view);

          glDrawArrays(GL_TRIANGLES, 0, NumVertices);
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
  1.0 * (GLfloat) h / (GLfloat) w, -1.0, 1.0);
  else  projection = Ortho(-1.0* (GLfloat) w / (GLfloat) h, 1.0 *
  (GLfloat) w / (GLfloat) h, -1.0, 1.0, -1.0, 1.0);
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
        GLfloat cameraSpeed = 0.01f;
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

      case 'Q':
      case 'q':
      cout << "Program is terminated." << endl;
      exit(0);
      break;

    }


    glutPostRedisplay();		// redraw the image now
  }

  void rotatePlanet(int rank, double speed){
    modelView[rank] = RotateY(speed)*modelView[rank];


  }

  void
  idle( void )
  {

      for(int i = 1; i < NumPlanet;i++){
        rotatePlanet(i,i*0.05);

      }

      glutPostRedisplay();
  }


int
main( int argc, char **argv )
{

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_RGBA | GLUT_DEPTH );
    glutInitWindowSize( 512, 512 );
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
    return 0;
}
