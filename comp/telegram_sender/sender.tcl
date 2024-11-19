#!/usr/bin/tclsh
#package provide telegram_sender 1.0
package require http
package require tls

http::register https 443 ::tls::socket
http::config -useragent {Tcl telegram bot sender}

# after 0 {SendMessageToTelegram $MESSAGE}

proc SendMessageToTelegram {message pathToCredentials} {
	array set cred [getCredentials $pathToCredentials]
	#set url "https://api.telegram.org/bot$cred(bot_token)/sendMessage"
	set url {http://127.0.0.1}
	set body "{\"text\": \"$message\", \"chat_id\": \"$cred(chat_id)\"}"
	set contentType application/json
	post $url $body $contentType onTelegramServerRespond
}

proc onTelegramServerRespond {body} {
	if {![hasOkInRespond $body]} {
		global telegram_sender_waiter
		set telegram_sender_waiter [list {The server reported an error while sending a message to Telegram.} 4]
	}
}

proc hasOkInRespond {body} {
	set isOk false
	foreach el [split $body ,] {
		set keyValue [split $el :]
		set key [lindex $keyValue 0]
		if {[string equal $key \"ok\"]} {
			set value [lindex $keyValue 1]
			if {[string equal $value true] || [string equal $value \"true\"]} {
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
		global telegram_sender_waiter
		set telegram_sender_waiter [list $errorOrToken 1]
	}
	after 5000 {
		if {[info exist errorOrToken]} {
			http::reset $errorOrToken
		}
		global telegram_sender_waiter
		set telegram_sender_waiter [list {The telegram server is not respond} 2]
	}

	proc onAnswer {callback token} {
		upvar #0 $token res
		if {[string match *200* $res(http)]} {
			uplevel #0 $callback $res(body)
		} else {
			global telegram_sender_waiter
			set telegram_sender_waiter [list "Server respond with error: $res(http)" 3]
		}
		http::cleanup $token
		global telegram_sender_waiter
		set telegram_sender_waiter [list {Message successfully sent to telegram.} 0]
	}
}

proc getCredentials {pathToCredentials} {
	set credStream [open $pathToCredentials r]
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
