#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>
#include "rft_util.h"
#include "rft_client_util.h"


/* 
 * UTILITY FUNCTIONS PROVIDED FOR YOU 
 * Do NOT edit the following functions:
 *      is_corrupted (local static function)
 *      pint_cmsg
 *      print_cerr
 *      exit_cerr
 */

/* 
 * is_corrupted - returns true with the given probability 
 * 
 * The result can be passed to the checksum function to "corrupt" a 
 * checksum with the given probability to simulate network errors in 
 * file transfer 
 */

/* print client information messge to stdout */
void print_cmsg(char *msg) {
    print_msg("CLIENT", msg);
}

/* print client error message to stderr */
void print_cerr(int line, char *msg) {
    print_err("CLIENT", line, msg);
}

/* exit with a client error */
void exit_cerr(int line, char *msg) {
    print_cerr(line, msg);
    exit(EXIT_FAILURE);
}


/*
 * FUNCTIONS THAT YOU HAVE TO IMPLEMENT
 */

/*
 * See documentation in rft_client_util.h
 * Hints:
 *  - Remember to print appropriate information/error messages for 
 *    success and failure to open the socket.
 *  - Look at server code.
 */
//TODO add messages
int create_udp_socket(struct sockaddr_in *server, char *server_addr, int port) {
    /* Replace the following with your function implementation */
    int sockfd = -1;


    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket failed");
        return -1;
    }

    server->sin_family = AF_INET; // IPv4
    server->sin_addr.s_addr = inet_addr(server_addr);
    server->sin_port = htons(port);


    errno = ENOSYS;

    return sockfd;
}

/*
 * See documentation in rft_client_util.h
 * Hints:
 *  - The metadata will have a copy of the output file name that the 
 *      server will use as the name of the file to write to
 *  - Remember to close any resource you are given if sending fails
 *  - Look at server code.
 */
//TODO add error and success messages
bool send_metadata(int sockfd, struct sockaddr_in *server, off_t file_size,
                   char *output_file) {
    /* Replace the following with your function implementation */


    metadata_t *file_meta = (metadata_t *) malloc(sizeof(metadata_t));
    file_meta->size = file_size;
    strcpy(file_meta->name, output_file);

    if (sendto(sockfd, file_meta, sizeof(metadata_t), 0,
               (struct sockaddr *) server, sizeof(struct sockaddr_in)) < 0) {
        printf("Unable to send message\n");
        return false;
    }


    errno = ENOSYS;
    free(file_meta);
    return true;
}


/*
 * See documentation in rft_client_util.h
 * Hints:
 *  - Remember to output appropriate information messages for the user to 
 *      follow progress of the transfer
 *  - Remember in this function you can exit with an error as long as you 
 *      close open resources you are given.
 *  - Look at server code.
 */
size_t send_file_normal(int sockfd, struct sockaddr_in *server, int infd,
                        size_t bytes_to_read) {

    char payload[PAYLOAD_SIZE];
    char buff[bytes_to_read];

    struct segment msg_payload;
    segment_t ack_rec;
    bool file_end = false;

    if (read(infd, buff, bytes_to_read) > 0) {
        int i;
        int size = 0;
        int sq = 0;

        for (i = 0; i <= bytes_to_read; ++i) {

            if (i == bytes_to_read) {
                file_end = true;
            }

            if (file_end || PAYLOAD_SIZE - 1 == size) {
                if (size > 0) {
                    payload[size] = '\0';


                    printf("%s", payload);



                    memset(msg_payload.payload, 0x00, size);
                    memcpy(msg_payload.payload, &payload, PAYLOAD_SIZE);

                    int cs = checksum(msg_payload.payload, false);
                    msg_payload.checksum = cs;
                    msg_payload.last = file_end;
                    msg_payload.payload_bytes = size;
                    msg_payload.sq = sq;

                    size = 0;

                    bool sending = true;
                    size_t seg_size = sizeof(segment_t);

                    while (sending) {
                        printf("%s\n", payload);

                        if (sendto(sockfd, &msg_payload, sizeof(struct segment), 0,
                                   (struct sockaddr *) server, sizeof(struct sockaddr_in)) < 0) {
                            printf("Unable to send message\n");
                            return 0;
                        }


                        memset(&ack_rec, 0, seg_size);
                        socklen_t addr_len = (socklen_t) sizeof(struct sockaddr_in);

                        if (recvfrom(sockfd, &ack_rec, seg_size, 0,
                                                             (struct sockaddr*) server, &addr_len) < 0){
                            printf("Couldn't receive\n");
                            return -1;
                        } else{
                            printf("Received ACK");
                            if (ack_rec.sq == sq) {
                                sending = false;
                                sq++;
                            }
                        }


                    }


                }

            }
            payload[i] = buff[i];
            size++;

        }
    }
    printf("%s\n", payload);


    return bytes_to_read;
}


/*
 * See documentation in rft_client_util.h
 * Hints:
 *  - Remember to output appropriate information messages for the user to 
 *      follow progress of the transfer
 *  - Remember in this function you can exit with an error as long as you 
 *      close open resources you are given.
 *  - Look at server code.
 */
size_t send_file_with_timeout(int sockfd, struct sockaddr_in *server, int infd,
                              size_t bytes_to_read, float loss_prob) {
    /* Replace the following with your function implementation */
    errno = ENOSYS;
    exit_cerr(__LINE__, "send_file_with_timeout is not implemented");

    return 0;
}



