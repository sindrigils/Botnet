# Botnet
Hello this is the README for the early submission, we have mostly implemented everything, all the client and server commands. If you are going to run the code, please look at the example down below, the client needs to authenticate with a string,
in order to communicate with the server, the password is "kaladin". And we the group number needs to be set when running the server, so the last parameter for tsamgroup5 is for example "A5_5".

In order to run the program to the following:
## make
## ./tsamgroup5 <port> <group-number>
## ./client <ip-address> <port>
## client: kaladin (You should receive a "Well hello there!" from the server)
## Now run whatevert command you would like to run, here are a few examples LISTSERVERS, CONNECT,IP,PORT, LISTUNKNOWN (servers that have not responded back with HELO)

The wireshark trace is in the wireshark.txt in root, there are also log files in the logs folder, which are logs from being connected on the tsam server and on my own computer (the server was not on tsam server)