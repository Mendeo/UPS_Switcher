#!/usr/bin/tclsh
package require http
package require tls

http::register https 443 ::tls::socket
http::config -useragent {Tcl telegram bot sender}

set MESSAGE {Receive poweroff command.}

after 0 {
	array set cred [getCredentials]
	set url "https://api.telegram.org/bot$cred(bot_token)/sendMessage"
	#set url {http://127.0.0.1}
	set body "{\"text\": \"$MESSAGE\", \"chat_id\": \"$cred(chat_id)\"}"
	set contentType application/json
	post $url $body $contentType onTelegramServerRespond
}

proc onTelegramServerRespond {body} {
	
	if {[hasOkInRespond $body]} {
		puts {Message successfully sent to telegram.}
	} else {
		puts {The server reported an error while sending a message to Telegram.}
	}
}

proc hasOkInRespond {body} {
	set isOk false
	foreach el [split $body ,] {
		set keyValue [split $el :]
		if {[string equal [lindex $keyValue 0] \"ok\"]} {
			if {[string equal [lindex $keyValue 1] true] || [string equal [lindex $keyValue 1] \"true\"]} {
				set isOk true
			} else {
				break
			}
		}
	}
	return $isOk
}

proc post {url body contenType callback} {
	catch {http::geturl $url -headers [list Content-Type $contenType] -query $body -command [list onAnswer $callback]} errorOrToken
	if {[string first ::http:: $errorOrToken] == -1} {
		puts $errorOrToken
		exit 1
	}
	after 5000 {
		puts {Server is not respond}
		if {[info exist errorOrToken]} {
			http::reset $errorOrToken
		}
		exit 2
	}

	proc onAnswer {callback token} {
		upvar #0 $token res
		if {[string match *200* $res(http)]} {
			uplevel #0 $callback $res(body)
		} else {
			puts "Server respond with error: $res(http)"
			exit 3
		}
		http::cleanup $token
		exitNormal
	}
}

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