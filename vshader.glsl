attribute   vec4 vPosition;
attribute   vec3 vNormal;

// output values will be interpretated per-fragment
varying  vec3 fN;
varying  vec3 fE;
varying  vec3 fL;

uniform mat4 ModelView;
uniform vec4 LightPosition;
uniform mat4 Projection;
uniform mat4 CameraView;

uniform int isSpace;


uniform int lighting;

varying vec4 texCoord;

void main()
{
    if (lighting == 1 && isSpace == 0){
   
        fN = (ModelView*vec4(vNormal, 0.0)).xyz; // normal direction in camera coordinates

        fE = (ModelView * vPosition).xyz; //viewer direction in camera coordinates

        fL = LightPosition.xyz; // light direction
    
        if( LightPosition.w != 0.0 ) {
            fL = LightPosition.xyz - fE;  //fixed light source
        }
    }

   texCoord = vPosition;
    if (isSpace == 1)
    	gl_Position = ModelView*vPosition;
    else
    	gl_Position = Projection*CameraView*ModelView*vPosition;
   
   
}
