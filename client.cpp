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

#include <ctime>

#include <stdlib.h>

#include <iostream>

#include <cstdlib>

#include <stdio.h>

//bufferUDP length larger than maximum UDP packet size
#define BUF_LEN 65540
#include <opencv2/highgui/highgui.hpp>

#include "opencv2/opencv.hpp"

#include "config.h"

using namespace cv;
using namespace std;

//UDP declarations
int sockDescUDP;
string servAddressUDP, myAddressUDP;
unsigned short servPortUDP, myPortUDP;

// Buffer for the received datagram packet
char bufferUDP[BUF_LEN];

// Size of received message
int recvMsgSizeUDP;

// Address of datagram source
string sourceAddressUDP;

// Port of datagram source
unsigned short sourcePortUDP;

int x;

//TCP declarations
#define FRAME_INTERVAL (1000/30)
#define BUFF_TAM 1024
struct sockaddr_in clienteTCP;
int puertoTCP, conexionTCP, servPortTCP, myPortTCP;
char bufferTCP[100];
int bytesTCP = 0;
int cbytesTCP;
int bytesTCPTam;
int ibufTCP[1];
string servAddressTCP;

//TCP client methods
void create_socketUDP() {
  //Socket creation
  if ((sockDescUDP = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {

    printf("Error al crear el socket\n");
    exit(1);

  }

  //Bind the socket to its port 
  sockaddr_in localAddr;
  memset( & localAddr, 0, sizeof(localAddr));
  localAddr.sin_family = AF_INET;
  localAddr.sin_addr.s_addr = inet_addr(myAddressUDP.c_str());
  localAddr.sin_port = htons(myPortUDP);

  if (bind(sockDescUDP, (sockaddr * ) & localAddr, sizeof(sockaddr_in)) < 0) {

    printf("Error al asociar el puerto al socket\n");
    close(sockDescUDP);
    exit(1);

  }
}

void send_message(raw_type * bufferUDP, size_t length) {
  sockaddr_in destAddr;

  // Zero out address structure
  memset( & destAddr, 0, sizeof(destAddr));

  // Internet address  
  destAddr.sin_family = AF_INET;  
  destAddr.sin_addr.s_addr = inet_addr(servAddressUDP.c_str());

  // Assign port in network byte order
  destAddr.sin_port = htons(servPortUDP);

  // send data to client
  if (sendto(sockDescUDP, (raw_type * ) bufferUDP, length, 0,
      (sockaddr * ) & destAddr, sizeof(destAddr)) != length) {

    printf("Send failed\n");
    exit(1);

  }
}

void receive_message(size_t length) {
  //Receive Message
  sockaddr_in clntAddr;
  socklen_t addrLen = sizeof(clntAddr);

  if ((recvMsgSizeUDP = recvfrom(sockDescUDP, (raw_type * ) bufferUDP, length, 0,
      (sockaddr * ) & clntAddr, (socklen_t * ) & addrLen)) < 0) {
    printf("Error al recibir mensaje\n");
    close(sockDescUDP);
    exit(1);
  }
}

void * mantain_connection(void * x_void_ptr) {
  int ibuf[1];
  ibuf[0] = 111;
  time_t last_message = time(NULL);
  while (1) {
    time_t current_time = time(NULL);
    if (current_time - last_message > 30) {
      send_message(ibuf, sizeof(int));
      last_message = current_time;
    }
  }

}

void cliente_udp(string servAddressUDP, int servPortUDP, string myAddressUDP, int myPortUDP) {
  create_socketUDP();

  namedWindow("recv", CV_WINDOW_AUTOSIZE);

  int ibuf[1];
  ibuf[0] = 111;

  send_message(ibuf, sizeof(int));

  //Defining thread for receiving data
  pthread_t connection_thread;

  if (pthread_create( & connection_thread, NULL, mantain_connection, & x)) {

    printf("Error creating thread\n");
    exit(1);

  }

  while (1) {

    // Block until receive message from server
    do {
      receive_message(sizeof(int));
    } while (recvMsgSizeUDP > sizeof(int));

    //The first message contains the amount of packets to receive
    int total_pack = ((int * ) bufferUDP)[0]; 

    //PACK_SIZE = udp pack size defined in config.h (4096)
    char * longbuf = new char[PACK_SIZE * total_pack];

    for (int i = 0; i < total_pack; i++) {
      receive_message(BUF_LEN);

      //Copies the values from the location pointed by the bufferUDP
      // directly to the memory block pointed by longbuff
      memcpy( & longbuf[i * PACK_SIZE], bufferUDP, PACK_SIZE);
    }

    Mat rawData = Mat(1, PACK_SIZE * total_pack, CV_8UC1, longbuf);
    Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);
    if (frame.size().width == 0) {
      printf("Decodificatio failed\n");
      continue;
    }
    imshow("recv", frame);
    free(longbuf);

    waitKey(1);
  }

  exit(1);
}

//UDP client methods
void create_socketTCP() {
  conexionTCP = socket(AF_INET, SOCK_STREAM, 0);
  puertoTCP = 1515; //puertoTCP servidor
  bzero((char * ) & clienteTCP, sizeof(clienteTCP));

  //set clientTCP
  clienteTCP.sin_family = AF_INET;
  clienteTCP.sin_port = htons(servPortTCP);
  clienteTCP.sin_addr.s_addr = inet_addr(servAddressTCP.c_str());

  //connect to socket
  if (connect(conexionTCP, (struct sockaddr * ) & clienteTCP, sizeof(clienteTCP)) < 0) {
    printf("Error conectando con el host\n");
    close(conexionTCP);
    exit(0);
  }
  printf("Conectado con %s:%d\n", inet_ntoa(clienteTCP.sin_addr), htons(clienteTCP.sin_port));
}

void receive_streamingTCP() {
  bytesTCP = 0;

  if ((bytesTCP = recv(conexionTCP, ibufTCP, sizeof(int), 0)) == -1) {
    printf("No se pudo recibir la imagen\n");
    close(conexionTCP);
    exit(0);
  } else {
    bytesTCPTam = ibufTCP[0];
  }

  bytesTCP = 0;
  char * longbuf = new char[bytesTCPTam];
  char sockData[bytesTCPTam];

  while (bytesTCP < bytesTCPTam) {
    if ((cbytesTCP = recv(conexionTCP, sockData, BUFF_TAM, 0)) == -1) {
      printf("No se pudo recibir la imagen\n");
      close(conexionTCP);
      exit(0);
    } else
    if (cbytesTCP > 0) {
      memcpy( & longbuf[bytesTCP], sockData, cbytesTCP);
      bytesTCP = bytesTCP + cbytesTCP;
    }
  }

  //array with image received  
  Mat rawData = Mat(1, bytesTCPTam, CV_8UC1, longbuf);

  //decode image
  Mat frame = imdecode(rawData, CV_LOAD_IMAGE_COLOR);

  //show image in clienteTCP window
  imshow("clienteTCP", frame);
  delete[] longbuf;
  waitKey(1);
}

void cliente_tcp(string server_ip, int server_port) {
  servAddressTCP = server_ip;
  servPortTCP = server_port;

  create_socketTCP();

  namedWindow("clienteTCP", CV_WINDOW_AUTOSIZE);

  while (true) {
    receive_streamingTCP();
  }

  close(conexionTCP);
  exit(1);
}

//Main client
int main(int argc, char * argv[]) {
  servAddressUDP = argv[1];
  servPortUDP = atoi(argv[2]);
  string protocol = argv[5];

  if (protocol == "UDP") {
    myAddressUDP = argv[3];
    myPortUDP = atoi(argv[4]); //third argument my port
    cliente_udp(servAddressUDP, servPortUDP, myAddressUDP, myPortUDP);
  } else {
    cliente_tcp(servAddressUDP, servPortUDP);
  }
}
