'use strict';
const { Gpio } = require('onoff');
const { exec } = require('child_process');

const POWER_OFF_COMMAND = 'poweroff';

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
	const powerOffPin = new Gpio(27, 'in', 'rising', { debounceTimeout: 10 });
	const powerStatusPin = new Gpio(22, 'out');
	powerStatusPin.writeSync(1);
	
	const isWaitingPowerOffSignal = true;
	powerOffPin.watch((err, value) => 
	{
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
