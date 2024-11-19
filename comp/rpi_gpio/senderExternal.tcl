#!/usr/bin/tclsh
#package require telegram_sender

source ../telegram_sender/sender.tcl
set MESSAGE {Receive poweroff command.}
set pathToCredentials ../telegram_sender/credentials.txt
set telegram_sender_waiter 0
after 0 {SendMessageToTelegram $MESSAGE $pathToCredentials}
vwait telegram_sender_waiter

puts [lindex $telegram_sender_waiter 0]
# if {[info exist error]} {
# 	puts $error
# } else {
# 	puts {Message successfully sent to telegram.}
# }


# after 4000 {puts qq}
# vwait qq