# Video Streaming Service - UDP & TCP 

The objective of this app is to make a video transmission application, which allows the exchange of information between multiple clients and a server. The protocols used for this app are UDP and TCP, where each client chooses the protocol they wish to use for streaming.

![Streaming Service](https://user-images.githubusercontent.com/54251435/63562484-2b7b8f80-c534-11e9-8fc0-36024da28f34.png)


## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development.

### Prerequisites

Before you run the app make sure to have the following things installed:
 - GCC compiler. 

```
$ sudo apt install gcc
```

 - OpenCV Library

```
$ sudo apt-get install libopencv-dev python3-opencv
```

 - CMake

```
 $ sudo apt-get -y install cmake 
```

### Installing

A step by step series of examples that tell you how to get a development env running.

Once you've cloned or downloaded the project, go to the project's folder and run the following commands:
```
$ cmake .
```
```
$ make
```
By now, you will have everything you need to run the app

### Running

First you should run the server 
```
$ ./server PUBLIC_IP  UDP_PORT TCP_PORT 
```
where PUBLIC_IP is the value of your public IP, UDP_PORT is the port where the server will be listening for requests for clients requesting transmition via UDP protocol, and same case with TCP_PORT

To check your public IP you can do so by executing the following command

```
$ ifconfig
``` 
eg.
```
$ ./server 192.168.1.52  6000 9000
``` 

By now the server should be up and running, waiting for clients requests to connect. 

For a client to request to be transmitted using TCP protocol you should run

```
$ ./client SERVER_PUBLIC_IP TCP_SERVER_PORT 
``` 
eg.
```
$ ./client 192.168.1.52 9000 
``` 


For a client to request to be transmitted using TCP protocol you should run

```
$ ./client SERVER_PUBLIC_IP UDP_SERVER_PORT MY_PUBLIC_IP MY_UDP_PORT UDP
``` 
eg.
```
$ ./client 192.168.1.52 6000 192.168.1.51 5000 UDP
``` 

> **IMPORTANT:** You can run the server and the clients in different PC's, with different IP's, as long as both processes are running in the same network.

> **NOTE:** By default, the video is going to be livestreamed straight from your webcam. You can choose to stream any video you want by setting the path to the video in line `342` from `server.cpp`. After making any change in the documents, re-compile the project by running the command `make` once again.

## Theoretical background

The application consists of a client / server network application architecture in which there is an active and known host (server) that serves the requests of other hosts (clients). It consists in two applications programmed in C ++ language, one that acts as a server (which will be the one that transmits the streaming) and another that acts as a client, which will be executed by all those who want to receive the data from the streaming server.

### TL;DR

### Multiple Connections UDP/TCP
In order to support multiple connected clients, we must keep record of which clients should we transmit the streaming frames. This was implemented using threads that are waiting for new connections (TCP) and waiting for renewal requests (UDP). Each time a client establishes a connection (TCP) or makes a streaming request (UDP), the server adds it to a client list.
In the case of TCP, there is a set that stores the numbers of connections established with the clients. For UDP, there is a map made up of a structure called Client, which has as its key the IP and port number of the client, and its value associated with the key is the time of the last streaming request.
It can be seen that the Client structure used to save the UDP clients is analogous to the TCP connection number, since in UDP there are no connections, the client data is saved to know to whom the packets should be sent, instead in TCP it reaches us with the connection number since this is unique per connection and has the associated data of server IP, server port, client IP, client port.

UDP also saves the time of the client's last streaming request, since it must make a new request every 30 seconds, in this way the server can control how long it does not receive new requests (for each client) and at the moment to send each frame, check the time that has elapsed since your last message and thus delete them from the list in case more than 90 seconds have passed.

### Completion of packet transmition via UDP
The UDP protocol does not establish a connection between clients and server, so the server stops sending packets, deleting the client from its list of receivers and thus ending the sending of packets. This will happen when the client has been more than 90 seconds without sending a streaming request message.

### Closing TCP connection
When a client wants to stop streaming, they close the connection. The client executes a given command to close the connection and then at the transport layer level the following happens:

 - The TCP client sends a segment with the END flag at 1 to the server, indicating that it wants to close the connection. 
 - When the TCP server receives it, it returns a segment that indicates that it received the message (ACK flag in 1) and sends a segment indicating that it also wants to close the connection (END flag in 1)
 - Finally, the client indicates to the server that it received its closing message by sending a confirmation segment (ACK flag in 1).
 - After this sequence, both hosts are released.

To verify that the sockets have been well closed, we use the ss command in the console, executing ss - tcp after closing the connection, we observe that the Local Adress: Port and Peer Adress: Port pair corresponding to the TCP connection established for the streaming is not present, so we conclude that it was well closed.

