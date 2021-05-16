#ifndef _RFT_CLIENT_H
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h> // for sockaddr_in

/*
 * INTRODUCTION AND WHAT YOU HAVE TO DO
 * For Part 1 of the assignment, you have to implement the following 
 * functions declared and documented in this file:
 *      create_udp_socket
 *      send_metadata
 *      send_file_normal
 *
 * For Part 2 of the assignment, you have to implement the following function 
 * that is declared and documented in this file:
 *      send_file_with_timeout
 * 
 * Do not change any function signatures and do not change any other functions
 * or code provided.
 *
 * You complete implementation of the functions in: rft_client_util.c
 *
 * That is, you do not edit this file. You edit the functions listed above
 * in rft_client_util.c
 */

/* 
 * create_udp_socket - create/open a UDP socket to communicate with the server
 *      with the server and fill out the server socket address information.
 *
 *      As a by-product of this function information and error messages 
 *      are printed for the user to follow progress of the file transfer 
 *      process. 
 *
 * Parameters:
 * server - a server sockaddr struct to fill out
 * server_addr - the server IP address (e.g. 127.0.0.1)
 * port - the port the server is listening on
 *
 * Return:
 * On success: a socket file descriptor to use for communication with the 
 *      server
 * On failure: -1
 */
int create_udp_socket(struct sockaddr_in* server, char* server_addr, int port);

/* 
 * send_metadata - send metadata (file size and file name to create) using 
 *      the given open socket to the server identified by the given sockaddr.
 *  
 *      This function does NOT print any information or error messages.
 *      If sending metadata fails, this function closes open resources 
 *      passed to it.
 *
 * Parameters:
 * sockfd - the socket file descriptor to use to send the metadata (created
 *      by create_udp_socket)
 * server - the server sockaddr struct (filled out by create_udp_socket)
 * port - the size of the file that is going to sent
 * output_file - the name of the file that the server will create for output
 *      of the data to be sent by the client (it will be a copy of the client's
 *      file)
 *
 * Return:
 * True if the metadata was successfully sent, false otherwise (and the 
 *      the function closes open resources passed to it)
 */
bool send_metadata(int sockfd, struct sockaddr_in* server, off_t file_size, 
    char* output_file);
    
/* 
 * send_file_normal - send the file represented by the given open file 
 *      descriptor, using the given open socket to the server identified 
 *      by the given sockaddr struct. The function returns the number of 
 *      bytes sent to the server.
 *
 *      The file is sent in chunks as payload to a succession of one or 
 *      more data segments. The number of segments required is determined 
 *      by the size of the file.
 *
 *      THE SERVER EXPECTS EACH CHUNK OF A FILE TO BE A CORRECTLY TERMINATED
 *      STRING. This function must guarantee this property for the payload
 *      it sends.
 *
 *      The main client function does not call send_file_normal if infd is
 *      empty.
 *
 *      This function has the following side effects:
 *      - information messages printed for the user to follow progress of the
 *          file transfer
 *      - exit of the client on detection of an error in the file transfer 
 *          process. Errors may arise from reading the file on the client 
 *          side or from sending the file to the server or from receiving 
 *          ACKS from the server.
 *          Open resources provided to the function are closed before any
 *          error exit.
 *
 * Parameters:
 * sockfd - the socket file descriptor to use to send the file (created
 *      by create_udp_socket)
 * server - the server sockaddr struct (filled out by create_udp_socket)
 * infd - open file descriptor of the client's input file to send to the 
 *      server
 * bytes_to_read - the number of bytes expected to be read form the file
 *      (initialised to the file size)
 *
 * Return:
 * On success: the number of bytes sent to the server
 * On failure: the function causes exit of the client with an error message
 */
size_t send_file_normal(int sockfd, struct sockaddr_in* server, int infd, 
    size_t bytes_to_read);

/* 
 * send_file_with_timeout - send the file represented by the given open file 
 *      descriptor, using the given open socket to the server identified 
 *      by the given sockaddr struct. The function returns the number of 
 *      bytes sent to the server.
 *      This function is essentially the same as the send_file_normal function
 *      except that it resends data segments if no ACK for a segment is 
 *      received from the server. The function implements this as follows:
 *      (i) it simulates network corruption or loss of data segments by
 *          injecting corruption into segment checksums (using the combination
 *          of the is_corrupted and checksum functions provided). The 
 *          probability of loss/corruption is determined by the loss_prob
 *          parameter.
 *      (ii) it times out when waiting to receive an ACK from the server
 *          for a data segment. The server does not ACK corrupted segments.
 *          Therefore, this client function will timeout waiting for an ACK 
 *          for a corrupted segment. When the timeout expires, the 
 *          function resends the data segment.
 *
 *      The file is sent in chunks as payload to a succession of one or 
 *      more data segments. The number of segments required is determined 
 *      by the size of the file.
 *
 *      THE SERVER EXPECTS EACH CHUNK OF A FILE TO BE A CORRECTLY TERMINATED
 *      STRING. This function must guarantee this property for the payload
 *      it sends.
 *      
 *      The main client function does not call send_file_normal if infd is
 *      empty.
 *      
 *      This function has the following side effects:
 *      - information messages printed for the user to follow progress of the
 *          file transfer
 *      - exit of the client on detection of an error in the file transfer 
 *          process. Errors may arise from reading the file on the client 
 *          side or from sending the file to the server or from receiving 
 *          ACKS from the server (that is, actual errors in receipt of ACKS as
 *          opposed to timeout of ACKS)
 *          Open resources provided to the function are closed before any
 *          error exit.
 *
 * Parameters:
 * sockfd - the socket file descriptor to use to send the file (created
 *      by create_udp_socket)
 * server - the server sockaddr struct (filled out by create_udp_socket)
 * infd - open file descriptor of the client's input file to send to the 
 *      server
 * bytes_to_read - the number of bytes expected to be read form the file
 *      (initialised to the file size)
 * loss_prob - the probability of the loss or corruption of a segment
 *
 * Return:
 * On success: the number of bytes sent to the server
 * On failure: the function causes exit of the client with an error message
 */
size_t send_file_with_timeout(int sockfd, struct sockaddr_in* server, int infd, 
    size_t bytes_to_read, float loss_prob);

/* 
 * Definition of utility function provided for you
 */
void print_cmsg(char* msg);             // print client information message
void print_cerr(int line, char* msg);   // print client error message
void exit_cerr(int line, char* msg);    // exit client with error message

#endif


