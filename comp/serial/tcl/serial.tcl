set serialport [open "com2" r+]
fconfigure $serialport -blocking 0 -buffering none -mode 9600,n,8,1 -translation binary -eofchar {}
fileevent $serialport readable onSerialInput

fconfigure stdin -blocking 0 -buffering none -translation lf -eofchar {}
fileevent stdin readable onSerialOutput

proc onSerialInput {} {
	global serialport
	set chunk [read $serialport]
	puts -nonewline $chunk
	flush stdout
}
proc onSerialOutput {} {
	global serialport
	set chunk [read stdin]
	puts -nonewline $serialport $chunk
}

vwait forever