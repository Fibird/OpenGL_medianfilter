uniform sampler2DRect texture;

void main(void)
{
	// Get the current texture location
	vec2 pos = gl_TexCoord[0].st;
	int mid = 0;
	int k = 0;
	int window[9];

	// Pick up window elements
	/*for (float i = pos.x - 1.0; i < pos.x + 2.0; i += 1.0)
	{
		for (float j = pos.y - 1.0; j < pos.y + 2.0; j += 1.0)
		{
			window[k++] = texture2DRect(texture, vec2(i, j));
		}
	}

	// Order elements (Only half of them)
	for (int m = 0; m < 5; ++m)
	{
		// Find position of minimum element
		int min = m;
		for (int n = m + 1; n < 9; ++n)
			if (window[n] < window[min])
				min = n;
		// Put found minimum element in its place
		const unsigned char temp = window[m];
		window[m] = window[min];
		window[min] = temp;		
	}				
	rst = window[4];*/
	gl_FragColor = 255 - texture2DRect(texture, vec2(i, j));
}