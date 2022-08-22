#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <unistd.h>

// Port to which UDP socket is bind to in the server.
#define UDP_PORT 1024

// ping packet size
#define PING_PKT_S 64

//Definition of ping packets. Zero Padded Message incluing the ICMP header
struct pingPacket{
    struct icmphdr hdr;
    char msg[PING_PKT_S-sizeof(struct icmphdr)];
};

int main(int argc, char** argv){
    if(argc!=2){
        perror("IP address required\n");
        exit(EXIT_FAILURE);
    }

    // The structure which specifies IPV4 tranport address and port number of client and server. 
    struct sockaddr_in address_server;
    memset(&address_server , 0 , sizeof(address_server));
    //This address family provides interprocess communication between processes that run on the same system or on different systems
    address_server.sin_family = AF_INET; 
    //Checks the Endians of the system
    address_server.sin_port = htons(UDP_PORT);

    // Checking if IP is valid and including server ip in server_addr structure.
    if(inet_aton(argv[1], &address_server.sin_addr)==0){
        perror("Hostname: Bad.\n");
        exit(EXIT_FAILURE);
    }

    // Creating a socket.
    // SOCK_DGRAM enables UDP transmission
    // Packets are sent through with ICMP protocol
    int sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_ICMP);
    if(sock_fd < 0){
        perror("ERROR opening listener socket.");
        return 0;
    }

    // Establish Connection between client and server
    int client_sockfd = connect(sock_fd ,(struct sockaddr*)&address_server , sizeof(address_server));
    if(client_sockfd<0){
        perror("CONNECT FAILED\n");
        exit(EXIT_FAILURE);
    }

    // Detecting the client network interface through which communication is
    // expected to happen.
    // The ip address and port details are stored in client_addr.
    struct sockaddr_in client_address;
    memset(&client_address , 0 , sizeof(client_address));
    client_address.sin_family = AF_INET;
    socklen_t addr_length = sizeof(client_address);
    if(getsockname(sock_fd, (struct sockaddr *)&client_address, &addr_length)<0){
        std::cout<<"Error detecting network interface.";
        exit(EXIT_FAILURE);
    }

    // Sets number of hops the packet exists in the network
    int time_to_live = 64;
    if(setsockopt(sock_fd, IPPROTO_IP ,IP_TTL, &time_to_live , sizeof(time_to_live))<0){
        perror("TTL option setting failed.");
        exit(EXIT_FAILURE);
    }

    // Setting receiver timeout.
    struct timeval t_out;
    t_out.tv_sec = 1;
    t_out.tv_usec = 0;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&t_out, sizeof(t_out)) != 0) {
        perror("Error setting receiver timeout.");
        exit(EXIT_FAILURE);
    }

    // Creating the packet to be sent.
    struct pingPacket ping_packet;
    memset(&ping_packet, 0, sizeof(ping_packet));
    ping_packet.hdr.type = ICMP_ECHO;
    ping_packet.hdr.code = 0;
    ping_packet.hdr.checksum = 0;
    ping_packet.hdr.un.echo.sequence = 1;

    // Sending an ICMP_ECHO packet to the server.
    struct timespec send_time, recieve_time;
    clock_gettime(CLOCK_MONOTONIC, &send_time);
    if(sendto(sock_fd, &ping_packet, sizeof(ping_packet), 0, (struct sockaddr *)&address_server, sizeof(address_server))<0){
        perror("Packet sending Failed.");
        exit(EXIT_FAILURE);
    }
    // Receiving an ICMP_REPLY packet from the server.
    if(recvfrom(sock_fd, &ping_packet, sizeof(pingPacket), 0, (struct sockaddr *)&client_address, &addr_length)<0){
        perror("Request timed out or host unreachable.");
        exit(EXIT_FAILURE);    
    }
    clock_gettime(CLOCK_MONOTONIC, &recieve_time);

    // Round trip time calculation.
    long double round_trip_time_msec = 0;
    if(ping_packet.hdr.code==ICMP_ECHOREPLY){
        // Computing RTT - round trip time.
        round_trip_time_msec = ((long double)(recieve_time.tv_nsec-send_time.tv_nsec))/1000000.0;
        round_trip_time_msec = round_trip_time_msec + (recieve_time.tv_sec-send_time.tv_sec)*1000.0;
        std::cout<<"Reply from "<<argv[1]<<" RTT = "<<round_trip_time_msec<<" ms\n";
    }
    else{
        std::cout<<"No ICMP reply!";
        exit(EXIT_FAILURE);
    }   
    close(sock_fd);
    exit(EXIT_SUCCESS);
}