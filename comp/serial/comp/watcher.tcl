set POWER_OFF_COMMAND poweroff
set POWER_OFF_ARGS {}

set serialport [open [lindex $argv 0] r+]
fconfigure $serialport -blocking 0 -buffering none -mode 9600,n,8,1 -translation crlf -eofchar {}
fileevent $serialport readable onSerialInput

proc sendAlive {} {
	global serialport
	puts -nonewline $serialport "+"
}

proc onSerialInput {} {
	global serialport
	set chunk [gets $serialport]
	# puts $chunk
	# flush stdout
	if { [string match *-* $chunk]} {
		puts {Receive poweoff signal}
		close $serialport
		global POWER_OFF_COMMAND
		global POWER_OFF_ARGS
		catch {exec $POWER_OFF_COMMAND {*}$POWER_OFF_ARGS} powerOffResult
		puts $powerOffResult
		flush stdout
		global exitProgram
		set exitProgram 1
	} elseif {[string match *a* $chunk]} {
		sendAlive
	}
}

vwait exitProgram