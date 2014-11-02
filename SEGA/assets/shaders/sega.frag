uniform sampler1D u_palette;

varying float v_color;

void main()
{	
	vec4 color = texture(u_palette, v_color).rgba;
	//vec4 color = texture1D(u_palette, v_color);
	gl_FragColor = color;

}