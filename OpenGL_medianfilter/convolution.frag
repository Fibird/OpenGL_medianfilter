uniform sampler2DRect texture;

void main(void)
{
	vec2 pos = gl_TexCoord[0].st;
	gl_FragColor = vec4(3, 3, 3, 1) - texture2DRect(texture, pos);
}