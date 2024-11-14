'use strict';

const fs = require('fs');
const fName = Math.floor(Math.random() * 10000) + '.txt';
const args = process.argv.slice(2).join(' ');
fs.writeFileSync(fName, args);
//console.log(args);

setInterval(() =>
{
	//console.log(Math.random());
	console.log('event: FALLING EDGE offset: 26 timestamp: [ 1769.057291908]');
}, 1000);