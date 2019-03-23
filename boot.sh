#!/bin/bash 

port=5001
server="server"
client="client"
func_ser()
{
	./server 192.168.1.214 $port
	if [ $? -eq 1 ]
	then
		$port = `expr $port + 1`
		func_ser
	fi

}
func_cet()
{
	./client 192.168.1.214 $port
	if [ $? -eq 1 ]
	then
		$port = `expr $port`
		func_cet
	fi
}
if [ "$1" == "$server" ]
then
	func_ser
elif [ "$1" == "$client" ]
then
	func_cet
fi

