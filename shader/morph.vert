/******************************************************************************/
/*                                                                            */
/* Vertex shader for morphing with vertex attribute                           */
/* Authors: Sangyoon Lee (sjames @ evl dot uic dot edu)                       */
/* Original idea from GPU Gems (used Cg)                                      */
/* Re-implemented in GLSL and add more features                               */
/*                                                                            */
/******************************************************************************/

attribute vec3 coordMorph0;
attribute vec3 normalMorph0;
attribute vec3 coordMorph1;
attribute vec3 normalMorph1;
attribute vec3 coordMorph2;
attribute vec3 normalMorph2;
attribute vec3 coordMorph3;
attribute vec3 normalMorph3;
attribute vec3 coordMorph4;
attribute vec3 normalMorph4;

uniform float morphWeight0;
uniform float morphWeight1;
uniform float morphWeight2;
uniform float morphWeight3;
uniform float morphWeight4;
uniform float useColorCode;

varying vec4 diffuse, ambient;
varying vec3 normal, lightDir, halfVector;
varying vec3 colorCode;

const float oneThird = 1.0 / 3.0;
	
void main()
{	
	// compute normal
	vec3 n;
	n = gl_Normal + morphWeight0 * normalMorph0
				  + morphWeight1 * normalMorph1
				  + morphWeight2 * normalMorph2
				  + morphWeight3 * normalMorph3
				  + morphWeight4 * normalMorph4;
				  
	normal = normalize(gl_NormalMatrix * n);
	
	lightDir = normalize(vec3(gl_LightSource[0].position));
	
	halfVector = normalize(gl_LightSource[0].halfVector.xyz);
					
	diffuse = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
	ambient = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;
		
	// compute morph delta position
	vec4 p;
	p.xyz =   morphWeight0 * coordMorph0
			+ morphWeight1 * coordMorph1
			+ morphWeight2 * coordMorph2
			+ morphWeight3 * coordMorph3
			+ morphWeight4 * coordMorph4;
	
	// copy delta value to color code
	colorCode = p.xyz;
	
	// compute final position value
	p.xyz += gl_Vertex.xyz; 	p.w = 1.0;
	gl_Position = gl_ModelViewProjectionMatrix * p;
	
	// compute color coding value based on scalar delta
	// sort of heat vision color spectrum
	// here just use 1.0 as full scale of delta (depends on model)
	float delta = length(colorCode);	
	if (delta < 1.0 / 3.0)
	{
		colorCode.x = 0.0; 
		colorCode.y = delta * 3.0; 
		colorCode.z = 1.0;
	}
	else if (delta > 2.0 / 3.0)
	{
		colorCode.x = 1.0; 
		colorCode.y = (1.0 - delta) * 3.0; 
		colorCode.z = 0.0;
	}
	else
	{
		colorCode.x = (delta - 1.0 / 3.0) * 3.0; 
		colorCode.y = 1.0; 
		colorCode.z = (2.0 / 3.0 - delta) * 3.0;
	}


	// finally apply whether we use color coding or not
	// useColorCode is uniform from app. value is either 0.0 or 1.0
	// guess that better than another if branch with boolean
	colorCode *= useColorCode;	
}