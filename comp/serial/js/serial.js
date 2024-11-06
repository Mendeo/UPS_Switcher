'use strict';
const { SerialPort } = require('serialport');
const serialport = new SerialPort({ path: 'com2', baudRate: 9600 });
serialport.on('data', (chunk) =>
{
	process.stdout.write(chunk);
});
process.stdin.on('data', (chunk) =>
{
	serialport.write(chunk);
});