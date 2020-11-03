// Rainbow RGB LED example

import("stdfaust.lib");

// From https://en.wikipedia.org/wiki/HSL_and_HSV#HSV_to_RGB
hsvToRgb(h, s, v) = r, g, b
with {
	h1 = h * 6;
	c1 = v * s;
	v1 = v;
	x = c1 * (1 - abs(h1 % 2 - 1));
	
	r1 = ba.if((h1 < 1), c1, 
		ba.if((h1 < 2), x,
		ba.if((h1 < 3), 0,
		ba.if((h1 < 4), 0,
		ba.if((h1 < 5), x, c1)))));
		  
	g1 = ba.if((h1 < 1), x, 
		ba.if((h1 < 2), c1,
		ba.if((h1 < 3), c1,
		ba.if((h1 < 4), x,
		ba.if((h1 < 5), 0, 0)))));
		  
	b1 = ba.if((h1 < 1), 0, 
		ba.if((h1 < 2), 0,
		ba.if((h1 < 3), x,
		ba.if((h1 < 4), c1,
		ba.if((h1 < 5), c1, x)))));

	m = v1 - c1;
	r = r1 + m;
	g = g1 + m;
	b = b1 + m;
};

process = par(i, 6, out(i+1))
with {
	phasor(freq) = freq/ma.SR : (+ : decimal) ~ _ with { decimal(x) = x-int(x); };
	out(i) = (sin(2 * ma.PI * h) * 5 + 5) <: (red(i), green(i), blue(i)) :> _
	with {
		h = (1 - i / 6 + phase) % 1;
		rgb = hsvToRgb(h, 1, 1);
		r = rgb : _,!,!;
		g = rgb : !,_,!;
		b = rgb : !,!,_;
		
		red(i,x) = attach(x, r : hbargraph("r1%i [switchlight_red:%i]", 0, 1) : hbargraph("r2%i [light_red:%i]", 0, 1));
		green(i,x) = attach(x, g : hbargraph("g1%i [switchlight_green:%i]", 0, 1) : hbargraph("g2%i [light_green:%i]", 0, 1));
		blue(i,x) = attach(x, b : hbargraph("b1%i [switchlight_blue:%i]", 0, 1) : hbargraph("b2%i [light_blue:%i]", 0, 1));
		
		phase = phasor(0.5);
	};
};