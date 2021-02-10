#!/bin/bash

i=4

while [ $i -gt 0 ]; do
	echo "$i"
	mosquitto_pub -h 192.168.2.7 -p 1883 -t socket_strip/set/relay/off -m 0
	sleep 0.1
	let i=i-1
done

exit

Error: Both topic and message must be supplied.
mosquitto_pub is a simple mqtt client that will publish a message on a single topic and exit.
mosquitto_pub version 1.5.7 running on libmosquitto 1.5.7.

Usage: mosquitto_pub {[-h host] [-p port] [-u username [-P password]] -t topic | -L URL}
                     {-f file | -l | -n | -m message}
                     [-c] [-k keepalive] [-q qos] [-r]
                     [-A bind_address]
                     [-i id] [-I id_prefix]
                     [-d] [--quiet]
                     [-M max_inflight]
                     [-u username [-P password]]
                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]
                     [{--cafile file | --capath dir} [--cert file] [--key file]
                      [--ciphers ciphers] [--insecure]]
                     [--psk hex-key --psk-identity identity [--ciphers ciphers]]
                     [--proxy socks-url]
       mosquitto_pub --help

 -A : bind the outgoing socket to this host/ip address. Use to control which interface
      the client communicates over.
 -d : enable debug messages.
 -f : send the contents of a file as the message.
 -h : mqtt host to connect to. Defaults to localhost.
 -i : id to use for this client. Defaults to mosquitto_pub_ appended with the process id.
 -I : define the client id as id_prefix appended with the process id. Useful for when the
      broker is using the clientid_prefixes option.
 -k : keep alive in seconds for this client. Defaults to 60.
 -L : specify user, password, hostname, port and topic as a URL in the form:
      mqtt(s)://[username[:password]@]host[:port]/topic
 -l : read messages from stdin, sending a separate message for each line.
 -m : message payload to send.
 -M : the maximum inflight messages for QoS 1/2..
 -n : send a null (zero length) message.
 -p : network port to connect to. Defaults to 1883 for plain MQTT and 8883 for MQTT over TLS.
 -P : provide a password
 -q : quality of service level to use for all messages. Defaults to 0.
 -r : message should be retained.
 -s : read message from stdin, sending the entire input as a message.
 -t : mqtt topic to publish to.
 -u : provide a username
 -V : specify the version of the MQTT protocol to use when connecting.
      Can be mqttv31 or mqttv311. Defaults to mqttv311.
 --help : display this message.
 --quiet : don't print error messages.
 --will-payload : payload for the client Will, which is sent by the broker in case of
                  unexpected disconnection. If not given and will-topic is set, a zero
                  length message will be sent.
 --will-qos : QoS level for the client Will.
 --will-retain : if given, make the client Will retained.
 --will-topic : the topic on which to publish the client Will.
 --cafile : path to a file containing trusted CA certificates to enable encrypted
            communication.
 --capath : path to a directory containing trusted CA certificates to enable encrypted
            communication.
 --cert : client certificate for authentication, if required by server.
 --key : client private key for authentication, if required by server.
 --ciphers : openssl compatible list of TLS ciphers to support.
 --tls-version : TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1.
                 Defaults to tlsv1.2 if available.
 --insecure : do not check that the server certificate hostname matches the remote
              hostname. Using this option means that you cannot be sure that the
              remote host is the server you wish to connect to and so is insecure.
              Do not use this option in a production environment.
 --psk : pre-shared-key in hexadecimal (no leading 0x) to enable TLS-PSK mode.
 --psk-identity : client identity string for TLS-PSK mode.
 --proxy : SOCKS5 proxy URL of the form:
           socks5h://[username[:password]@]hostname[:port]
           Only "none" and "username" authentication is supported.

See http://mosquitto.org/ for more information.

