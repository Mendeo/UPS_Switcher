set POWER_OFF_PIN 27
set POWER_STATUS_PIN 22
set BOUNCE_TIME 500
set POWER_OFF_COMMAND poweroff
set POWER_OFF_ARGS {}

if {[catch {exec gpioset -v}] || [catch {exec gpiomon -v}]} {
	puts {"gpiod" utilities not found}
	exit 1
}

proc write {pin value} {
	set progStream [open [list | gpioset -m signal 0 $pin=$value]]
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
	return $progStream
}

proc watch {handler pin} {
	set progStream [open [list | gpiomon -B pull-up $pin]]
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
	fileevent $progStream readable [list $handler $progStream]
	return $progStream
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

set onPowerOffSignalIsAlreadyLong_Id noid
proc onPowerOffPinChange {stream} {
	set chunk [read $stream]
	puts -nonewline $chunk
	flush stdout
	set value [checkValue $chunk]
	global onPowerOffSignalIsAlreadyLong_Id
	global BOUNCE_TIME
	#Сигнал инверсный
	if {!$value} {
		if {[string equal $onPowerOffSignalIsAlreadyLong_Id noid]} {
			set onPowerOffSignalIsAlreadyLong_Id [after $BOUNCE_TIME onPowerOffSignalIsAlreadyLong]
		}
	} else {
		if {![string equal $onPowerOffSignalIsAlreadyLong_Id noid]} {
			after cancel $onPowerOffSignalIsAlreadyLong_Id
			set onPowerOffSignalIsAlreadyLong_Id noid
		}
	}
}

puts {UPS power control enabled.}
set powerStatusStream [write $POWER_STATUS_PIN 0]
set powerOffPinChangeStream [watch onPowerOffPinChange $POWER_OFF_PIN]

proc onPowerOffSignalIsAlreadyLong {} {
	puts {Receive poweoff signal}
	global powerStatusStream
	global powerOffPinChangeStream
	catch {exec kill --signal SIGTERM [pid $powerStatusStream]} killResult
	puts $killResult
	catch {exec kill --signal SIGTERM [pid $powerOffPinChangeStream]} killResult
	puts $killResult
	close $powerStatusStream
	close $powerOffPinChangeStream
	global POWER_OFF_COMMAND
	global POWER_OFF_ARGS
	catch {exec $POWER_OFF_COMMAND {*}$POWER_OFF_ARGS} powerOffResult
	puts $powerOffResult
	flush stdout
	global exitProgram
	set exitProgram 1
}

vwait exitProgram
