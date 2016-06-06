// per-fragment interpolated values from the vertex shader
varying  vec3 fN;
varying  vec3 fL;
varying  vec3 fE;

uniform vec4 AmbientProduct, DiffuseProduct, SpecularProduct;
uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform float Shininess;
uniform int isSun;
uniform int isSpace;

uniform int lighting;

uniform sampler2D texture;
varying  vec4 texCoord;

void main() 
{ 

	vec2 longitudeLatitude;
	if(isSpace == 1)
		longitudeLatitude = vec2(texCoord.x, texCoord.y);
	else
		longitudeLatitude = vec2((atan(texCoord.y, texCoord.x) / 3.1415926 + 1.0) * 0.5,(asin(texCoord.z) / 3.1415926 + 0.5));

		if (lighting == 1 && isSpace == 0){
		// Normalize the input lighting vectors
		vec3 N = normalize(fN);
		vec3 E = normalize(fE);
		vec3 L = normalize(fL);

		vec3 H = normalize( L + E );

		vec4 ambient = AmbientProduct;

		float Kd = max(dot(L, N), 0.0);
		vec4 diffuse = Kd*DiffuseProduct;

		float Ks = pow(max(dot(N, H), 0.0), Shininess);
		vec4 specular = Ks*SpecularProduct;

		// discard the specular highlight if the light's behind the vertex
		if( dot(L, N) < 0.0 ) {
			specular = vec4(0.0, 0.0, 0.0, 1.0);
		}

		gl_FragColor = ambient + diffuse + specular;
		gl_FragColor.a = 1.0;
		gl_FragColor = gl_FragColor * texture2D( texture, longitudeLatitude );
		if(isSun == 1){
			gl_FragColor = texture2D( texture, longitudeLatitude );
		}

		// gl_FragColor.a = 0.0;

	} else{
	// vec2 longitudeLatitude = vec2((atan(texCoord.y, texCoord.x) / 3.1415926 + 1.0) * 0.5,(asin(texCoord.z) / 3.1415926 + 0.5));
	gl_FragColor = texture2D( texture, longitudeLatitude );
	//gl_FragColor = color;
		}
	//  
} 