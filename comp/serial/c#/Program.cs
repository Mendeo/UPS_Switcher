using System.IO.Ports;

string portName = Environment.GetCommandLineArgs()[1];
SerialPort port = new SerialPort(portName, 9600, Parity.None, 8, StopBits.One);
port.DataReceived += onSerialInput;
port.Open();

void onSerialInput(Object sender, SerialDataReceivedEventArgs e)
{
	string chunk = port.ReadExisting();
	Console.Write(chunk);
}

Task inputTask = new Task(() =>
{
	while (true) port.Write(Console.ReadLine() + "\r\n");
}, TaskCreationOptions.LongRunning);
inputTask.Start();
inputTask.Wait();
