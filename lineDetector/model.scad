$fn = 150;

lampD = 13;
resistorD = 12;
resistorPart1H = 8;
resistorPart2H = 13;
resistorPart3H = 15;
lampPart1H = 17;
lampPart2H = 25;
lampPart3H = 25;
wireResistorD = 2;
wireLampD = 3;
wireLampWithResistorD = 4;
wireDeltaResistor = 5;
wireDeltaLamp = 6;
wallTLamp = 4;
wallTResistor = 3;
lampCabelD = 5;
resistorCabelD = 3;

/*Отдельно для печати*/
//lampResistorPart1();
lampPart2();
//lampPart3();
//resistorPart2();
//resistorPart3();

/*Общий вид в сборе*/
/*
lampResistorPart1();
%translate([0, 0, -lampPart2H])
lampPart2();
translate([0, 0, -lampPart2H - lampPart3H])
lampPart3();

%translate([0, -resistorPart1H, lampPart1H / 2])
rotate([90, 0, 0])
resistorPart2();
translate([0, -resistorPart1H - resistorPart2H, lampPart1H / 2])
rotate([90, 0, 0])
resistorPart3();
*/

module lampResistorPart1()
{
	translate([0, 0, lampPart1H - wallTLamp])
	cylinder(h = wallTLamp, d = lampD);
	difference()
	{
		union()
		{
			cylinder(h = lampPart1H, d = lampD);
			translate([0, -(lampD / 2 - wallTLamp), lampPart1H / 2])
			rotate([90, 0, 0])
			cylinder(h = resistorPart1H - (lampD / 2 - wallTLamp), d = resistorD);
		}
		translate([0, 0, -1])
		cylinder(h = lampPart1H + 2, d = lampD - 2 * wallTLamp);
		translate([0, -(lampD / 2 - wallTLamp) + 1, lampPart1H / 2])
		rotate([90, 0, 0])
		cylinder(h = resistorPart1H + 1, d = resistorD - 2 * wallTResistor);
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
			cylinder(h = lampPart2H + 2, d = wireResistorD);
			translate([-wireDeltaResistor / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireResistorD);
		}
	}
}

module resistorPart3()
{
	difference()
	{
		cylinder(h = resistorPart3H, d = resistorD);
		translate([0, 0, -1])
		cylinder(h = resistorPart3H + 2, d = resistorCabelD);
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
			cylinder(h = lampPart2H + 2, d = wireLampD);
			translate([-wireDeltaLamp / 2, 0, 0])
			cylinder(h = lampPart2H + 2, d = wireLampWithResistorD);
		}
	}
}

module lampPart3()
{
	difference()
	{
		cylinder(h = lampPart3H, d = lampD);
		translate([0, 0, -1])
		cylinder(h = lampPart3H + 2, d = lampCabelD);
	}
}