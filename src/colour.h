#pragma once

struct Colour {
	Colour(float r, float g, float b)
		: r(r), g(g), b(b)
	{
		;
	}
	float r, g, b;
};

Colour boardColour(1.0, 0.7, 0.2);
Colour lineColour(0.1, 0.1, 0.1);
Colour blackColour(0, 0, 0);
Colour whiteColour(0.98, 0.98, 0.98);
