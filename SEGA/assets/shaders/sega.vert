uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;

attribute vec2 position;
attribute float color;

varying float v_color;

void main()
{
	v_color = color;

	vec4 position = vec4(position, 0, 1);	
	gl_Position = u_viewMatrix * (u_modelMatrix * position);
}