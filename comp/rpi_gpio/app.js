'use strict';
const { Gpio } = require('onoff');
const { exec } = require('child_process');

const POWER_OFF_COMMAND = 'poweroff';
const GPIO_REF = 512;

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
	const powerOffPin = new Gpio(27 + GPIO_REF, 'in', 'rising', { debounceTimeout: 10 });
	const powerStatusPin = new Gpio(22 + GPIO_REF, 'out');
	powerStatusPin.writeSync(1);
	
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
