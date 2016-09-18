/******************************************************************************/
/*                                                                            */
/* Vertex shader for morphing with vertex attribute                           */
/* Authors: Sangyoon Lee (sjames @ evl dot uic dot edu)                       */
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
uniform float unWrapWeight;
uniform int unWrapMode;

uniform float modelDepth0, modelDepth1;

varying vec4 diffuse, ambient;
varying vec3 normal, lightDir;
varying float gender;
	
void main()
{	
	vec3 n;
	vec4 p;
	
	// texture coord
	gl_TexCoord[0] = gl_MultiTexCoord0;
	
	// gender mix rate: will blend two texture in frag
	gender = morphWeight4;
	
	// compute normal
	n = gl_Normal + morphWeight0 * normalMorph0
				  + morphWeight1 * normalMorph1
				  + morphWeight2 * normalMorph2
				  + morphWeight3 * normalMorph3
				  + morphWeight4 * normalMorph4;
				  
	normal = normalize(gl_NormalMatrix * n);
	
	// light direction and material color
	lightDir = normalize(vec3(gl_LightSource[0].position));
	diffuse  = gl_FrontMaterial.diffuse * gl_LightSource[0].diffuse;
	ambient  = gl_FrontMaterial.ambient * gl_LightSource[0].ambient;
	ambient += gl_LightModel.ambient * gl_FrontMaterial.ambient;
		
	// compute morph delta position
	p.xyz =   morphWeight0 * coordMorph0
			+ morphWeight1 * coordMorph1
			+ morphWeight2 * coordMorph2
			+ morphWeight3 * coordMorph3
			+ morphWeight4 * coordMorph4;
	
	// compute common morph position
	p.xyz += gl_Vertex.xyz;
	
	// unWrap morphing
	if (unWrapWeight != 0.0)
	{
		float progress = 0.0;
		
		// pure linear interpolation
		if (unWrapMode == 0)
			progress = unWrapWeight;
		else
		{		
			// check whether this vtx should move at this moment
			// first, find current morphing plane in z direction
			float ratio;
			float cp = (modelDepth1 - modelDepth0) * unWrapWeight + modelDepth0;
			if (p.z < cp)
			{
				// clipping plane line changes: morphing is linear once it starts
				ratio = (p.z - modelDepth0) / (modelDepth1 - modelDepth0);
				progress = (unWrapWeight - ratio ) / (1.0 - ratio);
				
				// add some variation with sine wave form
				if (unWrapMode != 1)
					progress = sin(radians(progress * 90.0));

			}		
		}

		// normal
		n = (1.0 - progress) * n + progress * vec3(0.0,0.0,1.0);
		normal = normalize(gl_NormalMatrix * n);
			
		// position
		float offset;
		vec4 p1;
		p1.x = gl_TexCoord[0].s * 48.0; p1.x -= 12.0;
		p1.y = gl_TexCoord[0].t * 24.0; p1.y -= 10.5;
		p1.z = 9.265;
		
		// special attention for mode 3 (growing from ground)
		if (unWrapMode == 3)
			p1 = p1 * progress + (1.0 - progress) * p 
				- unWrapWeight*(modelDepth1 - modelDepth0)*vec4(0.0,0.0,1.0,0.0);
		else
			p1 = p1 * progress + (1.0 - progress) * p;
		
		p = p1;
		
	}
	
	// ok. now final position.
	p.w = 1.0;
	gl_Position = gl_ModelViewProjectionMatrix * p;
		
}