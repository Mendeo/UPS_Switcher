set powerOffPin 27
set powerStatusPin 22

proc write {pin value} {
	set progStream [open [list | node stub.js gpioset -m signal 0 $pin=$value]]
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
}

proc watch {handler pin} {
	set progStream [open [list | node stub.js gpiomon $pin]]
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
	fileevent $progStream readable [list $handler $progStream]
}

proc checkValue {rawStr} {
	if {[string match *FALLING* $rawStr]} {
		return 0
	}
	if {[string match *RISING* $rawStr]} {
		return 1
	}
	error {Unknow GPIO event}
}

proc onPowerOffPinChange {stream} {
	if {isWaitingPowerOffSignal} {
		set chunk [read $stream]
		set value [checkValue $chunk]
		if {value} {

		}
	}


}

puts {UPS power control enabled.}
write $powerStatusPin 0
set isWaitingPowerOffSignal true;
watch onPowerOffPinChange $powerOffPin






# set progCmd {node test.js}
# set progHandler [open |$progCmd]
# fconfigure $progHandler -blocking 0 -buffering none -translation binary -eofchar {}
# fileevent $progHandler readable onProgInput

# proc onProgInput {} {
# 	global progHandler
# 	set chunk [read $progHandler]
# 	puts -nonewline $chunk
# 	flush stdout
# }





vwait exitProgram

