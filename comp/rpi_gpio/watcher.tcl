#!/usr/bin/env tclsh

#Если сообщения в телеграм посылать не нужно, то нужно закоментировать следующие строки.
# set POWEROFF_MESSAGE {Receive poweroff command.}
# set POWERON_MESSAGE {Server is online.}
# set telegram_sender_dir [file normalize [file join [file dirname [info script]] ../telegram_sender]]
# source [file join $telegram_sender_dir sender.tcl]
# set TELEGRAM_CREDENTIALS [file join $telegram_sender_dir credentials.txt]
#######################################################################################

set POWER_OFF_PIN 27
set POWER_STATUS_PIN 22
set BOUNCE_TIME 500
set POWER_OFF_COMMAND poweroff
set POWER_OFF_ARGS {}

if {[catch {exec gpioset -v}] || [catch {exec gpiomon -v}]} {
	puts stderr {"gpiod" utilities are not found.}
	exit 1
}

proc write {pin value} {
	if {[catch {open [list | gpioset -m signal 0 $pin=$value 2>@stdout] r} progStream]} {
		puts stderr "Could not open gpio$pin: $progStream."
		exit 2
	}
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
	fileevent $progStream readable [list writeHandler $progStream]
	return $progStream
}

proc writeHandler {stream} {
	set chars [gets $stream chunk]
	if {$chars > 0 || $chars < 0} {
		global POWER_STATUS_PIN
		puts stderr "Could not open gpio$POWER_STATUS_PIN. $chunk"
		prepareToExit
		exit 3
	}
}

proc watch {handler pin} {
	if {[catch {open [list | gpiomon -b -B disable -F %e 0 $pin 2>@stdout] r} progStream]} {
		puts stderr "Could not open gpio$pin."
		exit 2
	}
	fconfigure $progStream -blocking 0 -buffering none -translation lf -eofchar {}
	fileevent $progStream readable [list $handler $progStream]
	return $progStream
}

set onPowerOffSignalIsAlreadyLong_Id noid
proc onPowerOffPinChange {stream} {
	set chars [gets $stream chunk]
	if {$chars > 0} {
		if {[scan $chunk %d value] == 0} {
			global POWER_OFF_PIN
			puts stderr "Could not open gpio$POWER_OFF_PIN: $chunk"
			prepareToExit
			exit 3
		}
		global onPowerOffSignalIsAlreadyLong_Id
		global BOUNCE_TIME
		if {$value} {
			if {[string equal $onPowerOffSignalIsAlreadyLong_Id noid]} {
				set onPowerOffSignalIsAlreadyLong_Id [after $BOUNCE_TIME onPowerOffSignalIsAlreadyLong]
			}
		} else {
			if {![string equal $onPowerOffSignalIsAlreadyLong_Id noid]} {
				after cancel $onPowerOffSignalIsAlreadyLong_Id
				set onPowerOffSignalIsAlreadyLong_Id noid
			}
		}
	} elseif {$chars < 0} {
		global POWER_OFF_PIN
		puts stderr "Could not open gpio$POWER_OFF_PIN"
		prepareToExit
		exit 3
	}
}

set powerStatusStream [write $POWER_STATUS_PIN 0]
set powerOffPinChangeStream [watch onPowerOffPinChange $POWER_OFF_PIN]
puts {UPS power control enabled.}
if {[info exists POWERON_MESSAGE]} {
	SendMessageToTelegram $POWERON_MESSAGE $TELEGRAM_CREDENTIALS
	vwait telegram_sender_waiter
	puts [lindex $telegram_sender_waiter 0]
}

proc prepareToExit {} {
	global powerStatusStream
	global powerOffPinChangeStream
	catch {exec kill --signal SIGTERM [pid $powerStatusStream]} killResult
	puts -nonewline $killResult
	catch {exec kill --signal SIGTERM [pid $powerOffPinChangeStream]} killResult
	puts -nonewline $killResult
	catch {close $powerStatusStream} errorMessage
	puts -nonewline $errorMessage
	catch {close $powerOffPinChangeStream} errorMessage
	puts -nonewline $errorMessage
}

proc onPowerOffSignalIsAlreadyLong {} {
	puts {Receive poweoff signal}
	prepareToExit
	global POWEROFF_MESSAGE
	if {[info exists POWEROFF_MESSAGE]} {
		global TELEGRAM_CREDENTIALS
		SendMessageToTelegram $POWEROFF_MESSAGE $TELEGRAM_CREDENTIALS
		global telegram_sender_waiter
		vwait telegram_sender_waiter
		puts [lindex $telegram_sender_waiter 0]
	}
	global POWER_OFF_COMMAND
	global POWER_OFF_ARGS
	catch {exec $POWER_OFF_COMMAND {*}$POWER_OFF_ARGS} powerOffResult
	puts $powerOffResult
	flush stdout
	global exitProgram
	set exitProgram 1
}

vwait exitProgram
