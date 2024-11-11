'use strict';
const { Gpio } = require('onoff');
const { exec } = require('child_process');

const POWER_OFF_COMMAND = 'poweroff';

/*Библиотека onoff не учитывает, что номер GPIO может быть смещён
  Смещение можно увидеть по пути /sys/class/gpio/
  Там будт файл gpiochip<номер смещения>
  Его нужно прописать в переменную GPIO_SHIFT
*/
const GPIO_SHIFT = 512;

if (Gpio.accessible)
{
	start();
}
else
{
	console.log('System dose not support GPIO.');
}

function start()
{
	const powerOffPin = new Gpio(27 + GPIO_SHIFT, 'in', 'rising', { debounceTimeout: 10 });
	const powerStatusPin = new Gpio(22 + GPIO_SHIFT, 'out');
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
