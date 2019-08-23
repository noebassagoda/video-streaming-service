// For data types
#include <sys/types.h>

// For socket(), connect(), send(), and recv()
#include <sys/socket.h>

// For gethostbyname()
#include <netdb.h>

// For inet_addr()
#include <arpa/inet.h>

// For close()
#include <unistd.h>

// For sockaddr_in
#include <netinet/in.h>

// Type used for raw data on this platform
typedef void raw_type;

// For errno
#include <errno.h>

// For memset
#include <string.h>

#include <iostream>

#include <cstdlib>

#include <stdio.h>

#include "config.h"

#include <map>

#include <pthread.h>

#include <mutex>

#include "opencv2/opencv.hpp"

//buffer length larger than maximum UDP packet size
#define BUF_LEN 65540

using namespace cv;
using namespace std;

//UDP Variables
int x;
int sockDescUDP, sockDescUDP2,serverPortUDP;
string servAddressUDP, serverIpUDP; 
unsigned short servPortUDP;

//mutex for accessing and modificating shared resource (clientsUDP map)
std::mutex mtxUDP;

struct ClientUDP {
      string ip;
      int port;
    } ;

//In order to have any struct as a key of a map, itUDP must be defined < operator
bool operator < (const ClientUDP &l, const ClientUDP &r) { return l.port < r.port; }

std::map<ClientUDP,time_t> clientsUDP, clientsUDP_copy;
std::map<ClientUDP,time_t>::iterator itUDP;


//TCP Variables
int conexion_servidorTCP, conexion_clienteTCP, puertoTCP;
socklen_t longc;
struct sockaddr_in servidor, cliente;
char bufferTCP[100];
std::set<int> clientsTCP, clientsTCP_copy;
std::set<int>::iterator itTCP;
std::mutex mtxTCP;
string servAddressTCP;


//UDP server methods
void create_socketUDP(){
  //Socket creation
  if ((sockDescUDP = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) { 
    
    printf("Error al crear el socket\n");
    exit(1);
  
  }

  //Bind the socket to itUDPs port 
  sockaddr_in localAddr;
  memset(&localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_port = htons(serverPortUDP);
  inet_aton(serverIpUDP.c_str(), &localAddr.sin_addr);
  
  if (bind(sockDescUDP, (sockaddr *) &localAddr, sizeof(sockaddr_in)) < 0) { 
  
    printf("Error al asociar el puerto al socket\n");
    close(sockDescUDP);
    exit(1);
  
  }
}

void *clientsUDP_connected(void *x_void_ptr) {
  create_socketUDP();

  // Buffer for the received datagram packet
  char buffer2[BUF_LEN];

  // Size of received message
  int recvMsgSize;

  // Address of datagram source
  string sourceAddress;

  // Port of datagram source
  unsigned short sourcePort;

  while(1){
    sockaddr_in clntAddr2;
    socklen_t addrLen2 = sizeof(clntAddr2);
    
    if ((recvMsgSize = recvfrom(sockDescUDP, (raw_type *) buffer2, BUF_LEN, 0, 
                        (sockaddr *) &clntAddr2, (socklen_t *) &addrLen2)) < 0) {
      exit(1);
    }

    ClientUDP c = ClientUDP();
    c.ip = inet_ntoa(clntAddr2.sin_addr);
    c.port = ntohs(clntAddr2.sin_port);

    mtxUDP.lock();
    clientsUDP[c]=time(NULL);
    mtxUDP.unlock();
      
  }
}

void send_messageUDP(String servAddressUDP, unsigned short servPortUDP, raw_type * buffer, size_t length){
  sockaddr_in destAddr;

  // Zero out address structure
  memset(&destAddr, 0, sizeof(destAddr));
  destAddr.sin_family = AF_INET;

  // Resolve name
  hostent *host;
  
  if ((host = gethostbyname(servAddressUDP.c_str())) == NULL) {
    
    printf("Error obtener nombre de host\n");
    exit(1);
  
  }
  
  destAddr.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
  // Assign port in network byte order
  destAddr.sin_port = htons(servPortUDP);

  // send data to client
 if (sendto(sockDescUDP, (raw_type *) buffer, length, 0,
              (sockaddr *) &destAddr, sizeof(destAddr)) != length) {
    
    printf("Send failed\n");
    exit(1);
  
  }
}

void send_UDP_Clients(vector < uchar > encoded){
  //Calculate amount of udp datagrams needed to send the hole frame
  int total_pack = 1 + (encoded.size() - 1) / PACK_SIZE;

  int ibuf[1];
  ibuf[0] = total_pack;
  
  mtxUDP.lock(); 
  //we work with a copy of the client list because a new client could
  //be added to the list while the frame already started sending
  clientsUDP_copy = clientsUDP;
  mtxUDP.unlock();

  for(itUDP=clientsUDP_copy.begin(); itUDP!=clientsUDP_copy.end(); ++itUDP){
    time_t current_time = time(NULL);
    ClientUDP c = itUDP->first;
    time_t last_message = itUDP->second;
    if ((current_time - last_message ) < 33){
      servAddressUDP = c.ip;
      servPortUDP = c.port;

      //Send amount of packets to client
      send_messageUDP(servAddressUDP,servPortUDP,ibuf,sizeof(int));

      for (int i = 0; i < total_pack; i++){
        send_messageUDP(servAddressUDP,servPortUDP, & encoded[i * PACK_SIZE], PACK_SIZE); //Send frame
      }
    }
    else{
      clientsUDP_copy.erase(itUDP);
    }
  }

  mtxUDP.lock();
  clientsUDP =  clientsUDP_copy;
  mtxUDP.unlock();
}

//TCP server methods
void create_socketTCP(){
    //Socket creation
    if ((conexion_servidorTCP = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

      printf("Error al crear el socket\n");
      close(conexion_servidorTCP);
      exit(1);
    }

    //Bind the socket to itTCPs port
    bzero((char *)&servidor, sizeof(servidor)); //llenamos la estructura de 0's
    servidor.sin_family = AF_INET; //asignamos a la estructura
    servidor.sin_port = htons(puertoTCP);
    inet_aton(servAddressTCP.c_str(), &servidor.sin_addr);

    if(bind(conexion_servidorTCP, (struct sockaddr *)&servidor, sizeof(servidor)) < 0){
      printf("Error al asociar el puertoTCP a la conexion\n");
      close(conexion_servidorTCP);
      exit(0);
    }

    if(listen(conexion_servidorTCP, 3) == -1){ 
      printf("Error al comenzar a escuchar\n");
      exit(1);
    }

    printf("A la escucha en el puertoTCP %d\n", ntohs(servidor.sin_port));
}

void connect_clientTCP(){
  if((conexion_clienteTCP = accept(conexion_servidorTCP, (struct sockaddr *)&cliente, &longc)) == -1){
    printf("Error al aceptar conexion\n");
    close(conexion_servidorTCP);
    exit(1);
  }
  else{
    printf("Server-accept() is OK...\n");
    mtxTCP.lock();

    //Inserts client to clientsTCP set
    clientsTCP.insert(conexion_clienteTCP);
    mtxTCP.unlock();
    printf("Connection stablished...\n");
  }
}

void *clientsTCP_connected(void *x_void_ptr) {
    create_socketTCP();

    longc = sizeof(cliente);

    while (1){
      connect_clientTCP();
    }

    close(conexion_servidorTCP);
    exit(1);
}

void send_TCP_Clients(vector < uchar > encoded){
  int bytes,cbytes;
  int jpegqual =  ENCODE_QUALITY; 
  int imgSize= encoded.size();

  mtxTCP.lock();
  //we work with a copy of the client list because a new client could
  //be added to the list while the frame already started sending
  clientsTCP_copy = clientsTCP;
  mtxTCP.unlock();
  
  for(itTCP=clientsTCP_copy.begin(); itTCP!=clientsTCP_copy.end(); ++itTCP){
    // Send image size
    int ibuf[1];
    ibuf[0] = imgSize;
    if (bytes = send(*itTCP,& ibuf[0], sizeof(int), MSG_NOSIGNAL) == -1){
      perror("TCP: Send");
      if (errno == 32) {
        close(*itTCP);
        clientsTCP.erase(*itTCP);
      }
    }

    // Send data here
    if (bytes = send(*itTCP,& encoded[0], imgSize, MSG_NOSIGNAL) < 0){
      perror("TCP: Send");
      if (errno == 32) {
        close(*itTCP);
        clientsTCP.erase(*itTCP);
      }
    }
  }
}

//Main server
int main(int argc, char * argv[]) {

  serverIpUDP = argv[1];
  servAddressTCP = argv[1];
  serverPortUDP = atoi(argv[2]);
  puertoTCP = atoi(argv[3]);

  //Defining thread for receiving data
  pthread_t incoming_clientsUDP_thread; 

  if(pthread_create(&incoming_clientsUDP_thread, NULL,clientsUDP_connected, &x)) {

    printf("Error creating thread\n");
    exit(1);

  }

  //Defining thread for receiving data
  pthread_t incoming_clientsTCP_thread;

  if(pthread_create(&incoming_clientsTCP_thread, NULL,clientsTCP_connected, &x)) {

    printf("Error creating thread\n");
    exit(1);

  }

  // Compression Parameter defined in config.h
  int jpegqual =  ENCODE_QUALITY; 

  Mat frame, send;
  vector < uchar > encoded;

  // Grab the camera with value 0,
  // or the path to a video
  VideoCapture cap("/home/geekside/Escritorio/tcp_udp/video.mp4"); 


  //clock_t last_cycle = clock();
  while (1) {
    cap >> frame;

    //simple integrity check; skip erroneous data...
    if(frame.size().width!=0){

      //Resize frame before sending, FRAME_WIDTH and FRAME_HEIGHT defined in config.h
      resize(frame, send, Size(FRAME_WIDTH, FRAME_HEIGHT), 0, 0, INTER_LINEAR);
      vector < int > compression_params;
      compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
      compression_params.push_back(jpegqual);

      //Encode image
      imencode(".jpg", send, encoded, compression_params);
      imshow("send", send);
      
      send_UDP_Clients(encoded);
      send_TCP_Clients(encoded);
    }
    waitKey(FRAME_INTERVAL);  //Waits FRAME_INTERVAL to continue excecution?
  }
  return 0;
}
