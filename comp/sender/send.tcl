#!/usr/bin/tclsh
package require http
package require tls

after 0 {
	array set cred [getCredentials]
	puts $cred(chat_id)

	http::register https 443 ::tls::socket
	set token [http::geturl https://mendeo.ru]
	puts $token
	upvar 0 $token qq
	foreach {key value} [array get qq] {
		puts "$key: $value"
	}
	http::cleanup $token
	exitNormal
} {}

proc getCredentials {} {
	set credStream [open credentials.txt r]
	set values {}
	while {![eof $credStream]} {
		set row [gets $credStream]
		if {[string length $row]} {
			set values [concat $values [split [string trim $row] =]]
		}
		
	}
	close $credStream
	return $values
}

proc exitNormal {} {
	global exitProgram
	set exitProgram 1
}

vwait exitProgram