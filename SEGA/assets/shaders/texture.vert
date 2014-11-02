uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;

attribute vec2 position;
attribute vec2 texCoords; 

varying vec2 v_texCoords;

void main()
{	
	vec4 coord = vec4(texCoords, 0.0, 1.0);
	v_texCoords = coord.xy;

	vec4 position = vec4(position, 0, 1);	
	gl_Position = u_viewMatrix * (u_modelMatrix * position);
}