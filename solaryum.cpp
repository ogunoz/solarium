#include "solar3D.h"

void readPPM(const char* textureName, int rank){

	FILE *fin;
	char tempstr[1024];
	fin = fopen(textureName, "rb");
	if (fin == NULL){
		cout << "Input file not found" << endl;
		cout << textureName << endl;
		return;
	}
	int maxLevel;

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
					image[wi-i-1][j][c] = aByte-0.00001; // moon r g b ayn� renkte oldu�u i�in b�yle yapt�k
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

void triangle( const point4& a, const point4& b, const point4& c){
	vec3  normal = normalize( cross(b - a, c - b) );

	normals[Index] = normal;  points[Index] = a;  Index++;
	normals[Index] = normal;  points[Index] = b;  Index++;
	normals[Index] = normal;  points[Index] = c;  Index++;

}

point4 unit( const point4& p ){
	float len = p.x*p.x + p.y*p.y + p.z*p.z;

	point4 t;
	if ( len > DivideByZeroTolerance ) {
		t = p / sqrt(len);
		t.w = 1.0;
	}

	return t;
}

void divide_triangle( const point4& a, const point4& b, const point4& c, int count){
	if ( count > 0 ) {
		point4 v1 = unit( a + b );
		point4 v2 = unit( a + c );
		point4 v3 = unit( b + c );
		divide_triangle(  a, v1, v2, count - 1);
		divide_triangle(  c, v2, v3, count - 1);
		divide_triangle(  b, v3, v1, count - 1);
		divide_triangle( v1, v3, v2, count - 1);
	}
	else {
		triangle( a, b, c);
	}
}

void tetrahedron( int count){
	point4 v[4] = {
			vec4( 0.0, 0.0, 1.0, 1.0 ),
			vec4( 0.0, 0.942809, -0.333333, 1.0 ),
			vec4( -0.816497, -0.471405, -0.333333, 1.0 ),
			vec4( 0.816497, -0.471405, -0.333333, 1.0 )
	};

	divide_triangle( v[0], v[1], v[2], count);
	divide_triangle( v[3], v[2], v[1], count);
	divide_triangle( v[0], v[3], v[1], count);
	divide_triangle( v[0], v[2], v[3], count);
}

void createPlanet(double radius, double distanceFromSun, int rank, const char* textureName){
	Index = 0;
	if(rank == Moon)
		translate[rank] = translate[Earth] + distanceFromSun/10;
	else
		translate[rank] = distanceFromSun;

	vec3 displacement = vec3(distanceFromSun, 0, 0); //displacement for each cube to seperate.
	mat4 model_view;

	mat4 scale;
	if(rank == SaturnRing)
		scale = Scale(radius*1.5, radius*1.5, radius*0.15);
	else
		scale = Scale(radius, radius, radius);

	model_view =   Translate(displacement) * scale;
	modelView[rank] = model_view;

	readPPM(textureName, rank);
}

// OpenGL initialization
void init(){
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );
	double screenX = glutGet(GLUT_WINDOW_WIDTH)/2;
	double screenY = glutGet(GLUT_WINDOW_HEIGHT)/2;

	for(int i = 0; i < NumPlanet; i++)
		rotationsOverSun[i] = 0;

	distances[Sun] = 0; distances[Mercury] = 0.57+0.038+1; distances[Venus] = 1.08+0.57+0.038+1;
	distances[Earth] = 1.08+0.57+0.038+1+1.50; distances[Mars] = 2.28+1.08+0.57+0.038+1+1.50;
	distances[Jupiter] = 7.79+2.28+1.08+0.57+0.038+1+1.50; distances[Saturn] = 14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[Uranus] = 28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1; distances[Neptune] = 45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50;
	distances[Pluto] = 59.10+45.50+28.80+14.30+7.79+2.28+1.08+0.57+0.038+1+1.50; distances[SaturnRing] = distances[Saturn]; distances[Moon] = 2;

	sunTurn[Sun] = 0; sunTurn[Mercury] = 87.97; sunTurn[Venus] = 224.7; sunTurn[Earth] = 365.26; sunTurn[Mars] = 686.98; sunTurn[Jupiter] = 4331.98;
	sunTurn[Saturn] = 10760.56; sunTurn[Uranus] = 30707.41; sunTurn[Neptune] = 60202.15; sunTurn[Pluto] = 90803.64; sunTurn[SaturnRing] = sunTurn[Saturn];
	sunTurn[Moon] = sunTurn[Earth];

	ownTurn[Sun] = 28; ownTurn[Mercury] = 58.65; ownTurn[Venus] = 243.01; ownTurn[Earth] = 1; ownTurn[Mars] = 1.0257; ownTurn[Jupiter] = 0.414;
	ownTurn[Saturn] = 0.444; ownTurn[Uranus] = 0.718; ownTurn[Neptune] = 0.671; ownTurn[Pluto] = 6.39; ownTurn[SaturnRing] = ownTurn[Saturn]; ownTurn[Moon] = 27.32;

	inclination[Sun] = 0; inclination[Mercury] = 0; inclination[Venus] = 178; inclination[Earth] = 23.4; inclination[Mars] = 25; inclination[Jupiter] = 3.08;
	inclination[Saturn] = 26.7; inclination[Uranus] = 97.9; inclination[Neptune] = 26.9; inclination[Pluto] = 122.5; inclination[SaturnRing] = inclination[Saturn];
	inclination[Moon] = 0;

	radiuses[Sun] = 1.5; radiuses[Mercury] = 0.038; radiuses[Venus] = 0.095; radiuses[Earth] = 0.1; radiuses[Moon] = 0.27;
	radiuses[Mars] = 0.053; radiuses[Jupiter] = 1.119; radiuses[Saturn] = 0.940; radiuses[Uranus] = 0.404; radiuses[Neptune] = 0.388; radiuses[Pluto] = 0.018;
	radiuses[SaturnRing] = 0.940;

	textureNames[Sun] = "sunmap.ppm"; textureNames[Mercury] = "mercurymap.ppm"; textureNames[Venus] = "venusmap.ppm";
	textureNames[Earth] = "earthmap1k.ppm"; textureNames[Moon] = "moon.ppm"; textureNames[Mars] = "mars_1k_color.ppm"; textureNames[Jupiter] = "jupitermap.ppm";
	textureNames[Saturn] = "saturn.ppm"; textureNames[Uranus] = "uranusmap.ppm"; textureNames[Neptune] = "neptunemap.ppm"; textureNames[Pluto] = "plutomap1k.ppm";
	textureNames[SaturnRing] = "saturnring.ppm";


	tetrahedron( NumTimesToSubdivide);

	imageGeneral = (GLubyte**) malloc(sizeof(GLubyte*) * NumPlanet);
	for(int i = 0; i < NumPlanet; i++)
		createPlanet(radiuses[i], distances[i], i, textureNames[i]);


	readPPM("space.ppm", NumPlanet);
	GLintptr offset = 0;

	double sizeY = 1;
	double sizeX = sizeY * screenX / screenY;

	points[NumVertices] = vec4(-sizeX, -sizeY, 0.0, 1.0);
	points[NumVertices + 1] = vec4(-sizeX, sizeY, 0.0, 1.0);
	points[NumVertices + 2] = vec4(sizeX, sizeY, 0.0, 1.0);
	points[NumVertices + 3] = vec4(sizeX, -sizeY, 0.0, 1.0);
	points[NumVertices + 4] = vec4(-sizeX, -sizeY, 0.0, 1.0);
	points[NumVertices + 5] = vec4(sizeX, sizeY, 0.0, 1.0);

	GLuint buffer;
	glGenBuffers( 1, &buffer );
	glBindBuffer( GL_ARRAY_BUFFER, buffer );
	glBufferData( GL_ARRAY_BUFFER, sizeof(points) + sizeof(normals), NULL, GL_STATIC_DRAW );

	//glBufferData( GL_ARRAY_BUFFER, sizeof(vertices), NULL, GL_STATIC_DRAW );

	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(points), points );
	offset+= sizeof(points);
	glBufferSubData( GL_ARRAY_BUFFER, offset, sizeof(normals), normals );

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

	// Retrieve transformation uniform variable locations*/
	ModelView = glGetUniformLocation( program, "ModelView" );
	Projection = glGetUniformLocation( program, "Projection" );
	CameraView = glGetUniformLocation( program, "CameraView" );

	glEnable( GL_DEPTH_TEST );
	glEnable(GL_CULL_FACE); //to discard invisible faces from rendering
	glEnable(GL_STENCIL_TEST);

	glClearColor( 1.0, 1.0, 1.0, 1.0 ); /* black background */
	glutPostRedisplay();
}

//----------------------------------------------------------------------------

void display( void ){

	glClearStencil(0);
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	mat4 mm[NumPlanet];

	glm::vec3 target = cameraPos + cameraFront;
	glm::mat4 cameraView;
	if(projection){

		double h = glutGet(GLUT_WINDOW_HEIGHT);
		double w = glutGet(GLUT_WINDOW_WIDTH);
		mat4  projection;
		if(w >= h){
			projection = Ortho(-0.135*cameraPos.z* (GLfloat) w / (GLfloat) h, 0.135 * cameraPos.z* (GLfloat) w / (GLfloat) h, -0.135* cameraPos.z, 0.135* cameraPos.z, -1000.0,1000.0);
		}
		else{
			projection = Ortho(-0.135*cameraPos.z, 0.135*cameraPos.z, -0.135*cameraPos.z * (GLfloat) h / (GLfloat) w, 0.135 *cameraPos.z* (GLfloat) h / (GLfloat) w, -100.0, 100.0);
		}

		glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );

	}

	cameraView = glm::lookAt(cameraPos, target, cameraUp);

	glUniformMatrix4fv(CameraView, 1, GL_FALSE, glm::value_ptr(cameraView));

	for (int k = 0; k < NumPlanet; k++) {
		glUniform1i( glGetUniformLocation(program, "isSun"), 0 );
		if(k == Sun)
			glUniform1i( glGetUniformLocation(program, "isSun"), 1 );

		glStencilFunc(GL_ALWAYS, k,0);

		mm[k] = RotateX(Theta[Xaxis]) * RotateY(Theta[Yaxis]) * RotateZ(Theta[Zaxis]) * modelView[k];

		if(k == Moon){
			glUniformMatrix4fv(ModelView, 1, GL_TRUE,mm[Earth] *  mm[k]);
		}
		else
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
		glDrawArrays(GL_TRIANGLES, 0, NumVertices);
	}

	//if(projection)
	glUniformMatrix4fv(ModelView, 1, GL_TRUE, Scale(1, 1, 1) * RotateX(-180) * RotateY(0) * RotateZ(0)  * Translate(0,0,-0.999999));
	//else{
	//	glUniformMatrix4fv(ModelView, 1, GL_TRUE,  Scale(1, 1, 1)  *  RotateX(-180) * RotateY(0) * RotateZ(0) * Translate(0,0,-0.9999999));

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width[NumPlanet], height[NumPlanet] , 0, GL_RGB, GL_UNSIGNED_BYTE, imageGeneral[NumPlanet]);

	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_LINEAR);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glActiveTexture( GL_TEXTURE0);

	glUniform1i( glGetUniformLocation(program, "isSpace"), 1);
	glDrawArrays(GL_TRIANGLES, NumVertices, 6);
	glUniform1i( glGetUniformLocation(program, "isSpace"), 0);
	glutSwapBuffers();
}

void reshape( int w, int h ){
	glViewport( 0, 0, w, h );

	if(!projection){

		GLfloat aspect;
		if (w < h){
			aspect = GLfloat(h)/w;
		}
		else{
			aspect = GLfloat(w)/h;
		}

		mat4  projection;
		projection = Perspective(15.0, aspect, 0.1, 1000.0f);
		glUniformMatrix4fv( Projection, 1, GL_TRUE, projection );
	}
	glutPostRedisplay();
}

//Mouse callback function.
void mouse( int button, int state, int x, int y ){

	GLfloat cameraSpeed = 0.1f;
	if ( state == GLUT_UP ) {
		switch( button ) {

		case 3:
			cameraPos += cameraSpeed * cameraFront;
			if(cameraPos.z < 0)
				cameraPos -= cameraSpeed * cameraFront;
			break; //Scroll-up
		case 4:
			cameraPos -= cameraSpeed * cameraFront;
			break; //Scroll-down
		}

		glutPostRedisplay();
	}
	else if ( state == GLUT_DOWN){
		if(button == GLUT_LEFT_BUTTON) {
			valid = 0;
			y = glutGet( GLUT_WINDOW_HEIGHT ) - y;

			unsigned char pixel[4];
			glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
			index = -1;

			if(pixel[0] != pixel[1] || pixel[1] != pixel[2]){

				glReadPixels(x, y, 1, 1, GL_STENCIL_INDEX, GL_UNSIGNED_INT, &index);

				if(index == SaturnRing)
					index = Saturn;



				if (index != lastSelection){

					changeUnit = translate[index] * cos(glm::radians(rotationsOverSun[index]));
					if(!follow){
						initCameraPos = cameraPos;
						anim_count = anim = 20;
						timer(5);
					}
					else{
						double ySin = sin(glm::radians(rotationsOverSun[index]));
						if(projection)
							cameraPos = glm::vec3 (changeUnit, (30 * radiuses[index] / cameraFront.z) - ySin*0.175 * index, 30 * radiuses[index]);
						else
							cameraPos = glm::vec3 (changeUnit, (60 * radiuses[index] / cameraFront.z) - ySin*0.175 * index , 60 * radiuses[index]);
					}
					lastSelection = index;
				}

				glutPostRedisplay();
			}

		}
		else if(button == GLUT_RIGHT_BUTTON){
			old_x = x;
			old_y = y;

			valid = 1;
		}
	}
}

void specialKeyboard(int k, int x, int y){
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

void keyboard(unsigned char k, int x, int y){
	GLfloat cameraSpeed = 0.2;
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

		if (in!= lastSelection){
			if(!follow){
				keyboard('*',0,0);
				index = in;
				changeUnit = translate[index] * cos(glm::radians(rotationsOverSun[index]));
				initCameraPos = cameraPos;
				anim_count = anim = 20;

				timer(5);
				lastSelection = in;
			}
			else{
				index = in;
				double ySin = sin(glm::radians(rotationsOverSun[index]));
				if(projection)
					cameraPos = glm::vec3 (changeUnit, (30 * radiuses[index] / cameraFront.z) - ySin*0.175 * index, 30 * radiuses[index]);
				else
					cameraPos = glm::vec3 (changeUnit, (60 * radiuses[index] / cameraFront.z) - ySin*0.175 * index , 60 * radiuses[index]);
			}

		}

		break;
	case 13:
		if(isFullScreen){
			glutPositionWindow(0,0);
			glutReshapeWindow(1600, 600);
		}
		else
			glutFullScreen();
		isFullScreen = !isFullScreen;
		break;
	case '*':
		cameraPos   = glm::vec3(0.0, -5.0,  40.0);
		cameraFront = glm::vec3(0.0, 1.0, -8.0);
		cameraUp    = glm::vec3(0.0, 1.0,  0.0);
		lastSelection = -1;

		break;
		//
	case 'n':
		projection = !projection;
		reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));

		break;
	case 'A':
	case 'a':   //Initialize the object to default angle.
		cameraPos -= normalize(cross(cameraFront, cameraUp)) * cameraSpeed;

		break;
	case 'S':
	case 's': //Initialize the object to default zoom value.
		cameraPos.y -= cameraSpeed;

		break;
	case 'W':
	case 'w': //Initialize the object to default zoom value.
		cameraPos.y +=  cameraSpeed;

		break;
	case 'D':
	case 'd': //Initialize the object to default zoom value.
		cameraPos += normalize(cross(cameraFront, cameraUp)) * cameraSpeed;

		break;
	case 'L':
	case 'l':
		if (lighting == true) lighting = false;
		else lighting = true;
		glUniform1i( Lighting, lighting );
		break;

	case '+':
		rotationSpeed += 30;
		lastSpeed = rotationSpeed;
		break;
	case '-':
		if(rotationSpeed >= 30){
			rotationSpeed -= 30;
			lastSpeed = rotationSpeed;
		}
		break;

	case 'P':
	case 'p':
		if(rotationSpeed == 0){
			rotationSpeed = lastSpeed;
			follow = true;
		}
		else{
			follow = false;
			rotationSpeed = 0;
		}
		break;

	case 'i':
	case 'I':
		rotationSpeed = initSpeed;
		projection = true;
		follow = false;
		keyboard('*',0,0);

		reshape(glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
		break;

	case 'f':
	case 'F':
		follow = !follow;
		break;

	case 'Q':
	case 'q':
		for(int i = 0; i < NumPlanet+1; i++){
			free(imageGeneral[i]);
		}
		free(imageGeneral);
		cout << "Program is terminated." << endl;
		exit(0);
		break;

	}
	glutPostRedisplay();		// redraw the image now
}

void rotatePlanet(int rank){
	if (rank != Sun){
		double angle = fmod(-rotationSpeed/sunTurn[rank], 360);
		rotationsOverSun[rank] += angle;

		if(rank != Moon)
			modelView[rank] =  RotateZ(-rotationSpeed/sunTurn[rank])*modelView[rank] * RotateZ(rotationSpeed/ownTurn[rank]);
		else
			modelView[Moon] = RotateY(-rotationSpeed/sunTurn[rank])*  modelView[Moon];

	}
	else
		modelView[rank] = modelView[rank] * RotateZ(rotationSpeed/ownTurn[rank]);
}

void idle( void ){

	for(int i = Sun; i < NumPlanet;i++){
			rotatePlanet(i);
	}
	//double y = sin(glm::radians(rotationsOverSun[index]));


	//cout <<radiuses[index] << endl;
	if(follow){
		changeUnit = translate[index] * cos(glm::radians(rotationsOverSun[index]));
		//	if(projection)
		cameraPos.x = changeUnit;
		//cameraPos = glm::vec3 (changeUnit, (30 * radiuses[index] / cameraFront.z) - y*0.175 * index, 30 * radiuses[index]);
		//	else
		//		cameraPos = glm::vec3 (changeUnit, (60 * radiuses[index] / cameraFront.z) - y*5.25*radiuses[index] , 60 * radiuses[index]);
	}

	glutPostRedisplay();
}

void timer( int p ){

	if(anim_count > 0){
		anim_count--;

		if(index == Moon)
			cameraPos = cameraPos + glm::vec3 ((changeUnit - initCameraPos.x)/anim, (7.5 * radiuses[Earth] / cameraFront.z - initCameraPos.y)/anim  , (7.5*radiuses[Earth] - initCameraPos.z)/anim);
		else
			cameraPos = cameraPos + glm::vec3 ((changeUnit - initCameraPos.x)/anim, (7.5 * radiuses[index] / cameraFront.z - initCameraPos.y)/anim  , (7.5*radiuses[index] - initCameraPos.z)/anim);


		glutPostRedisplay();
		glutTimerFunc(10,timer,p);

	}
}
/*
void mouseDrag (int x , int y){
	if (valid) {

		int xoffset = old_x - x;
		int yoffset = old_y - y;
		GLfloat sensitivity = 0.05f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw   += xoffset;
		pitch += yoffset;


		glm::vec3 front;
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = -sin(glm::radians(pitch));

		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		cameraFront = glm::normalize(front);


		old_x = x;
		old_y = y;
	}
	glutPostRedisplay();


}
 */

int main( int argc, char **argv ){

	glutInit( &argc, argv );
	glutInitDisplayMode( GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH );
	glutInitWindowSize( 1600, 600 );
	glutCreateWindow( "Solar System" );
	glewExperimental = GL_TRUE;
	glewInit();

	old_x = glutGet(GLUT_WINDOW_WIDTH)/2;
	old_y = glutGet(GLUT_WINDOW_HEIGHT)/2;

	init();

	glutMouseFunc( mouse );// set mouse callback function for mouse
	glutSpecialFunc(specialKeyboard); // set mouse callback function for mouse
	glutDisplayFunc( display );
	glutReshapeFunc( reshape );
	glutKeyboardFunc( keyboard );
	glutIdleFunc( idle );
	//glutMotionFunc(mouseDrag);

	glutMainLoop();
	for(int i = 0; i < NumPlanet+1; i++){
		free(imageGeneral[i]);
	}
	free(imageGeneral);
	return 0;
}

