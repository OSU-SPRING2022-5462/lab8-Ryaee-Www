# lab-8-Ryaee-Www - YuChen Gu

About Config file: The **First Line** contains >[Row Number] [Column Number]. All other line should have only one pair of server and port, excess pairs will be ignored. Any invalid entry will be **skipped and indicated to the user**. Program will continue with the remaining valid remote server. Total valid entries will be labelled after the loading process.

## Format of config file entries:
  
--all lines: all lines contains remote hosts ip, port and location

>[RemoteIP]  [RemotePortNum] [RemoteLocationNum]
  
**Every line should contain only one remote host**

## Run the program:
  
>[Make all (or make server4)] 
>
>[server4] [port number]

||**Upon program run**

Upon running the program, the program will Load all remote info from the config file.

After that all fetched host from config file will be listed (and invalid host, if any). The program will attempt to locate the port number inputted by the user.

Then the program will idle. 

||**Sending message**

If the user input anything on the console, the program will send this message to all valid remote host in config file in following format:
[version (currently, 4)]  [CurrentLocation]  [Target PortNumber] [Source (or local) PortNumber] [Hopcount] [message type (MSG, ACK or MOV)] [MSG ID] [Route (all remote host on route] [MessageBody]
  
Message is limited to 100 character long, if exceeded, the program will notice the user and return to idle state. It will not attempt to send invalid message.

The program will send to **ALL** valid remote host **Except** any remote host existed in the Route, regardless of their distance.

||**Receiving message**
  
If any message is sent to the listened host, the program will retrieve the version, and location of the sender.  If the distance between the sender and local host is greater than two (integer mathematics, round downward), this message will be labelled **OUT OF RANGE**, otherwise it will be labelled **IN RANGE** **Excepte for Message with type MOV (more detail follows)** Different message type will be treated differently.

**If the program did not find the port number in the config file, it will ask if the user will proceed or quit.**
>If the user choose to proceed, a location of 0 will be assigned to the local host. 0 is out of bound, meaning all its message, sent or received will be all labelled as OOR.

>In case the program found a out of range locNum corresponding the port, it will inidicate to the user and continue (the drone will still be OOR). 

||**Message Types**
If the local host received a message, it will first check its version number. The current version is 4, but the program support all previous versions.

Then the program will check if the message is a MOV message type. If so, it will then check if this message is for local host. If not, this message will be notified to the console and then ignored. Otherwise, the program will change its location base on the content stored in message body. **A MOV command will be executed regardless the distance from the command source host**

If the message is a MSG type, if the message is not for local host, it will be forwarded to all host except the ones on the route. Otherwise, it will be presented to the console if upstream remote host is within range. An ACK will be sent as a reply to the original remote host that sent this message. **ACK type will have the same message ID as the received ID**

If the message is an ACK type, if the message is not for local host, it will be forwarded to all host except the ones on the route. Otherwise, it is an reply to a message previously sent by local host. In this case, this message will not be presented fully, but instead acknowledge the use by present to the console in format "Message ID [MSG ID] to Port [Targetted port] was confirmed received"

||**additional info about message ID**
A message ID is coordinated by a sender,receiver Pair (it will be described as "local ID" in following texts). All valide remote host will have an attribute called message ID. It will be initialized as 0, meaning no communication ever happened between local host, and this specific remote host. ID will start at 1. In order to not show duplicate message on screen, Local host as receiver will only the message with Incomming ID larger than local ID will be shown on the screen. Local host as sender will only update an local ID upon received an ACK.

Example1: Remote host A initiated a first transmission, it will have an message ID as 1. For local host, since it has never communicate with A before, will have a local msgID as 0. Upon receive that message, the local MsgID will be update to 1 and replied with an ACK with ID 1. All remaining duplicated message forwarded to local host will still have an ID of 1. These message will all be ignored.

Example2: Local host initiate a first transmission to remote host A. Local ID is 0, but a message with ID 1 will be sent. A received a message with ID 1, its local ID will be updated to 1 from 0. Then A will reply with an ACK with ID 1. Local host then receive an ACK from A with ID 1, its local ID will update from 0 to 1. 

>If as a sender, local host never received an ACK, the sequence ID will never be updated. The second attempt to transmit to the same remote host will still have an ID of 1

