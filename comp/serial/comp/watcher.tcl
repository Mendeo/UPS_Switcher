set POWER_OFF_COMMAND poweroff
set POWER_OFF_ARGS {}

set serialport [open [lindex $argv 0] r+]
fconfigure $serialport -blocking 0 -buffering none -mode 9600,n,8,1 -translation binary -eofchar {}
fileevent $serialport readable onSerialInput

proc sendAlive {} {
	global serialport
	puts -nonewline $serialport "+"
}

set needSendAliveOn true
proc onSerialInput {} {
	global serialport
	set chunk [read $serialport]
	if { [string match *-* $chunk]} {
		close $serialport
		catch {exec $POWER_OFF_COMMAND {*}$POWER_OFF_ARGS} powerOffResult
		puts $powerOffResult
		flush stdout
		global exitProgram
		set exitProgram 1
	} elseif {[string match *a* $chunk]} {
		global needSendAliveOn
		if {$needSendAliveOn} {
			set needSendAliveOn false
			sendAlive
		}
	}
	# puts -nonewline $chunk
	# flush stdout
}

vwait exitProgram