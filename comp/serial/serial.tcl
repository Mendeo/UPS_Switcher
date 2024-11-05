set serialport [open [lindex $argv 0] r+]
fconfigure $serialport -blocking 0 -buffering none -mode 9600,n,8,1 -translation binary -eofchar {}

proc sendAlive {} {
	global serialport
	puts -nonewline $serialport "+"
	after 500 sendAlive
}

set needSendAliveOn true
proc onSerialInput {} {
	global serialport
	set chunk [read $serialport]
	if { [string equal $chunk "s"]} {
		catch {exec poweroff} result
		puts $result
		global exitProgram
		set exitProgram 1
	} elseif {[string equal $chunk "a"]} {
		global needSendAliveOn
		if {$needSendAliveOn} {
			set needSendAliveOn false
			sendAlive
		}
	}
	# puts -nonewline $chunk
	# flush stdout
}

fileevent $serialport readable onSerialInput
vwait exitProgram