uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform vec4 u_colorTransform;

attribute vec2 position;
attribute vec4 color;

varying vec4 v_color;

void main()
{
	v_color = color;// * u_colorTransform;
	
   vec4 position = vec4(position, 0, 1);
	gl_Position = u_viewMatrix * (u_modelMatrix * position);
}