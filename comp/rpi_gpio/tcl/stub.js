'use strict';

const fs = require('fs');
const fName = Math.floor(Math.random() * 10000) + '.txt';
const args = process.argv.slice(2).join(' ');
//fs.writeFileSync(fName, args);
//console.log(args);

let i = 0;
setInterval(() =>
{
	if (process.argv[2] === 'gpiomon')
	{
		if (i < 10)
		{
			console.log('event: FALLING EDGE offset: 26 timestamp: [ 1769.057291908]');
		}
		else
		{
			console.log('event: RISING EDGE offset: 26 timestamp: [ 1769.057291908]');
		}
		i++;
	}
	else
	{
		//console.log(Math.random());
	}
}, 1000);