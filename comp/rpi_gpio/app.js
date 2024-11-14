'use strict';
const { Gpio } = require('onoff');
const { exec } = require('child_process');
const fs = require('fs');

const POWER_OFF_COMMAND = 'poweroff';

/*Библиотека onoff не учитывает, что номер GPIO может быть смещён
  Смещение можно увидеть по пути /sys/class/gpio/
  Там будт файл gpiochip<номер смещения>
  Его нужно прописать в переменную GPIO_SHIFT
*/

if (Gpio.accessible)
{
	//Находим смещение GPIO
	fs.readdir('/sys/class/gpio/', (err, gpioFiles) =>
	{
		if (err)
		{
			console.log('Error when trying to find GPIO shift: ' + err.message);
		}
		else
		{
			const gpioShift = Math.min(...gpioFiles.filter(f => f.startsWith('gpiochip')).map(f => Number(f.slice(8))));
			start(gpioShift);
		}
	});
}
else
{
	console.log('System dose not support GPIO.');
}

function start(gpioShift)
{
	console.log('UPS power control enabled.');
	const powerOffPin = new Gpio(27 + gpioShift, 'in', 'rising', { debounceTimeout: 10 });
	const powerStatusPin = new Gpio(22 + gpioShift, 'out');
	powerStatusPin.writeSync(0);

	let isWaitingPowerOffSignal = true;
	powerOffPin.watch((err, value) =>
	{
		console.log('Receive poweoff signal');
		if (isWaitingPowerOffSignal)
		{
			if (err)
			{
				console.log(err);
			}
			else if (value)
			{
				isWaitingPowerOffSignal = false;
				setTimeout(() =>
				{
					if (powerOffPin.readSync())
					{
						console.log('Powering off');
						exec(POWER_OFF_COMMAND, (err, stdout, stderr) =>
						{
							if (err || stderr)
							{
								console.log('Failed to shutdown the system. ' + err?.message);
							}
						});
					}
					else
					{
						isWaitingPowerOffSignal = true;
					}
				}, 10000);
			}
		}
	});
}
