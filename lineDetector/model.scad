$fn = 50;

lampD = 13;
resistorD = 10;
resistorPart1H = 10;
resistorPart2H = 10;
lampPart1H = 15;
lampPart2H = 10;
wireResD = 3;
wireD = 1;
wireDeltaResistor = 3;
wireDeltaLamp = 4;
wallT = 2;

lampResistorPart1();
translate([0, 0, -lampPart2H])
%lampPart2();

%translate([0, -(resistorPart1H + lampD / 2), lampPart1H / 2])
rotate([90, 0, 0])
resistorPart2();



module lampResistorPart1()
{
	translate([0, 0, lampPart1H - wallT])
	cylinder(h = wallT, d = lampD);
	difference()
	{
		union()
		{
			cylinder(h = lampPart1H, d = lampD);
			translate([0, lampD / 2 - wallT * 1.2, lampPart1H / 2])
			rotate([90, 0, 0])
			cylinder(h = resistorPart1H + lampD / 2 - wallT * 1.2 + lampD / 2, d = resistorD);
		}
		translate([0, 0, -1])
		cylinder(h = lampPart1H + 2, d = lampD - 2 * wallT);
		translate([0, 0, lampPart1H / 2])
		rotate([90, 0, 0])
		cylinder(h = resistorPart1H + lampD / 2 + 1, d = resistorD - 2 * wallT);
	}
}

module resistorPart2()
{
	difference()
	{
		cylinder(h = resistorPart2H, d = resistorD);
		translate([0, 0, -1])
		{
			translate([wireDeltaResistor / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireD);
			translate([-wireDeltaResistor / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireD);
		}
	}
}

module lampPart2()
{
	difference()
	{
		cylinder(h = lampPart2H, d = lampD);
		translate([0, 0, -1])
		{
			translate([wireDeltaLamp / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireD);
			translate([-wireDeltaLamp / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireResD);
		}
	}
}