# Botnet - Group A5_5
Hello TA, this README includes all the bonus points we did, and how to start the assignment and some documentation on how it works. For starters we are on a MacOS and Windows (WSL2), and the it works
best to compile it on MacOS. Now here are instructions on how to start the project.

```
make
./tsamgroup5 <port>
./client <ip> <port>
```

Our client will connect to the server and send a HELO,kaladin command, which 

Now start by writing HELP in the clinet terminal, there you can see all the available commands you can use, and you can just start by connecting to some instructor server with the CONNECT command.
The server reguarly sends out HELO/KEEPALIVE/STATUSREQ, and each time it gets a STATUSRESP it checks if the other server has any messages for a server that we are connected if so, take 
it and forward it to the server that should get it. This allows us to send messages to servers 2-hops away. We also drop all connections that don't send a HELO within 5seconds.

The project is very modular, each file has its own claass that handles its own logic.  
PollManager - handles the poll logic, and owns the array of fds.  
ServerManager - handles the map of all servers, and provides information about what servers we are connected to.  
ConnectionManager - he handles all the logic regarding sockets, so like new connections, sending, and receiving.  
GroupMessageManager - has a map of all the stored messages and for which group its for, making it easy to keep track of all the stored messages.  
ServerCommands - handles all the commands from other servers.    
ClientCommands - handles all the commands from our client.  
Then we have like Logger, Server and an utils file, which are all pretty self explanatory.  


# BONUS POINTS
1. **We submitted our code early in Gradescope along with a Wireshark trace (1pts)**
2. **Obtain bonus points for connectivity**
    Here are two groups from AK, group number A5_52 and A5_53, we talked over discord and sent each other messages (0.5pts * 2 = 1pts)

    [2024-10-24 11:36:06] [RECEIVED] A5_52 (4052): SENDMSG,A5_5,A5_52,Hvað eru þið reykjaviku pakk að gera af ykkur núna?
    [2024-10-24 11:36:36] [SENDING] A5_52 (4052): SENDMSG,A5_52,A5_5,Allavana ekki að vinna i cyber
    
    [2024-10-24 15:40:58] [SENDING] A5_53 (4153): SENDMSG,A5_53,A5_5,Hello my friends, can you send a message back to me?
    [2024-10-24 15:43:59] [RECEIVED] A5_53 (4153): SENDMSG,A5_5,A5_53,yessir. This is our response

    **1 point per 5 different groups you can show messages received from** (10 groups = 2pts)  
    For each bracket ([]) are we receiving a message from a group and sending them a message.
   
    [1]  
    [2024-10-24 20:56:53] [RECEIVED] A5_69 (4069): SENDMSG,A5_5,A5_69,Hello A5_5 . You've successfully connected to group A5_69! Meaning that you've       sent both a valid HELO message and a SERVERS message. How are you?  
    [2024-10-24 20:57:13] [SENDING] A5_69(4069): SENDMSG,A5_69,A5_5,Once again, im doing just fine thanks  

    [2]  
        [2024-10-24 20:56:51] [RECEIVED] N/A (4003): SENDMSG,A5_5,A5_3,Hello from A5_3!!!  
        [2024-10-24 20:59:42] [SENDING] A5_3(4003): SENDMSG,A5_3,A5_5,Hello there!  

    [3]  
        [2024-10-24 21:03:44] [RECEIVED] A5_74 (4074): SENDMSG,A5_5,A5_74,blessaður gilsari  
        [2024-10-24 21:04:04] [SENDING] A5_74(4074): SENDMSG,A5_74,A5_5,hahah thank you meistari

    [4]  
        [2024-10-23 12:53:25] Received RAW from A5_7 (89.160.201.68:5001): SENDMSG,A5_5,A5_7,prufa ertu afatta NUMBER?  
        [2024-10-23 11:51:11] Sending to A5_7(89.160.201.68:5001): SENDMSG,A5_7,ORACLE,
        “If the automobile had followed the same development cycle as the computer, a Rolls-Royce 
         would today cost $100, get a million miles per gallon, and explode once a year, killing 
         everyone inside.”
        - Robert X. Cringely  

    [5]  
        [2024-10-23 15:54:16] Sending to A5_20(130.208.246.249:4020): SENDMSG,A5_20,A5_5,Cats or dogs?  
        [2024-10-24 09:44:57] Received RAW from A5_20 (130.208.246.249:4020): SENDMSG,A5_5,A5_20,Hello from A5_2  

    [6]  
        [2024-10-24 21:35:08] [RECEIVED] A5_100 (4100): SENDMSG,A5_5,A5_100,Jess erum tengdir núna!  
        [2024-10-24 21:36:00] [SENDING] A5_100(4100): SENDMSG,A5_100,A5_5,þ, þarna!! noice  


We also saw in the assignment description that it mentions if you can read the messages from the Number servers. Well we collected all the messages from him and crack it, here is what we found out: