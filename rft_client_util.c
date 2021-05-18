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

static bool is_corrupted(float prob) {
    float r = (float) rand();
    float max = (float) RAND_MAX;

    return (r / max) <= prob;
}

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

int create_udp_socket(struct sockaddr_in *server, char *server_addr, int port) {
    /* Replace the following with your function implementation */
    int sockfd = -1;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        exit_cerr(__LINE__, "Failed to open socket");
        return -1;
    }

    server->sin_family = AF_INET; // IPv4
    server->sin_addr.s_addr = inet_addr(server_addr);
    server->sin_port = htons(port);
    print_cmsg("Socket created");
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
bool send_metadata(int sockfd, struct sockaddr_in *server, off_t file_size,
                   char *output_file) {

    metadata_t *file_meta = (metadata_t *) malloc(sizeof(metadata_t));
    file_meta->size = file_size;
    memcpy(file_meta->name, output_file, FILE_NAME_SIZE);

    size_t bytes_sent = sendto(sockfd, file_meta, sizeof(metadata_t), 0,
                               (struct sockaddr *) server, sizeof(struct sockaddr_in));
    if (bytes_sent < 0) {
        close(sockfd);
        free(file_meta);
        return false;
    } else if (!bytes_sent) {
        close(sockfd);
        free(file_meta);
        return false;
    } else {
        free(file_meta);
        print_cmsg("Start sending file");
        print_sep();
        print_sep();
        return true;
    }

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
    char inf_msg_buf[INF_MSG_SIZE];
    char buff[bytes_to_read];
    struct segment msg_payload;
    int pay_count = 0;
    int sq = 0;
    int total_sent = 0;
    segment_t ack_rec;
    bool file_end = false;

    //Clear Payload before first iteration in case limit not reached
    memset(payload, 0x00, PAYLOAD_SIZE);

    if (read(infd, buff, bytes_to_read) > 0) {

        for (int i = 0; i <= bytes_to_read; ++i) {

            if (i == bytes_to_read) {
                file_end = true;
            }

            if (file_end || PAYLOAD_SIZE - 1 == pay_count) {

                memset(msg_payload.payload, 0x00, PAYLOAD_SIZE);
                memcpy(msg_payload.payload, &payload, pay_count + 1);

                int cs = checksum(msg_payload.payload, false);
                msg_payload.checksum = cs;
                msg_payload.last = file_end;
                msg_payload.payload_bytes = pay_count;
                msg_payload.sq = sq;

                total_sent += pay_count;
                pay_count = 0;
                bool sending = true;

                size_t seg_size = sizeof(segment_t);
                socklen_t addr_len = (socklen_t) sizeof(struct sockaddr_in);

                while (sending) {
                    snprintf(inf_msg_buf, INF_MSG_SIZE, "Sending segment with sq: %d, payload bytes: %zu, "
                                                        "checksum: %d", msg_payload.sq, msg_payload.payload_bytes,
                             msg_payload.checksum);
                    print_cmsg(inf_msg_buf);

                    ssize_t payload_bytes = sendto(sockfd, &msg_payload, sizeof(struct segment), 0,
                                                   (struct sockaddr *) server, addr_len);

                    if (payload_bytes < 0) {
                        close(infd);
                        close(sockfd);
                        exit_cerr(__LINE__, "Sending Payload error");
                    } else if (!payload_bytes) {
                        print_cmsg("Ending Connection");
                        sending = false;

                    } else {
                        snprintf(inf_msg_buf, INF_MSG_SIZE, "Sent payload: \n%s", msg_payload.payload);
                        print_cmsg(inf_msg_buf);
                    }
                    print_sep();
                    print_sep();

                    memset(&ack_rec, 0, seg_size);
                    print_cmsg("Waiting for an ack");

                    ssize_t ack_bytes = recvfrom(sockfd, &ack_rec, seg_size, 0,
                                                 (struct sockaddr *) server, &addr_len);
                    if (ack_bytes < 0) {
                        close(infd);
                        close(sockfd);
                        exit_cerr(__LINE__, "ACK Receive Failure");
                    } else if (!ack_bytes) {
                        errno = ENOMSG;
                        close(infd);
                        close(sockfd);
                        exit_cerr(__LINE__, "Ending connection - no ACK received");
                    } else {
                        snprintf(inf_msg_buf, INF_MSG_SIZE, "ACK with sq: %d Received", ack_rec.sq);
                        print_cmsg(inf_msg_buf);

                        sending = false;
                        memset(payload, 0x00, PAYLOAD_SIZE);
                        sq++;
                        print_sep();
                        print_sep();
                    }
                }

            }
            payload[pay_count] = buff[i];
            pay_count++;
        }
    } else {
        errno = ENODATA;
        close(infd);
        close(sockfd);
        exit_cerr(__LINE__, "Failed to read file");
    }
    close(infd);
    close(sockfd);
    return total_sent;
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
    char payload[PAYLOAD_SIZE];
    char inf_msg_buf[INF_MSG_SIZE];
    char buff[bytes_to_read];
    struct segment msg_payload;
    int total_sent = 0;
    int pay_count = 0;
    int sq = 0;
    segment_t ack_rec;
    bool file_end = false;

    // Define and set timeout
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    //Clear Payload before first iteration in case limit not reached
    memset(payload, 0x00, PAYLOAD_SIZE);

    if (read(infd, buff, bytes_to_read) > 0) {

        for (int i = 0; i <= bytes_to_read; ++i) {

            if (i == bytes_to_read) {
                file_end = true;
            }


            if (file_end || PAYLOAD_SIZE - 1 == pay_count) {

                memset(msg_payload.payload, 0x00, PAYLOAD_SIZE);
                memcpy(msg_payload.payload, &payload, pay_count + 1);

                msg_payload.last = file_end;
                msg_payload.payload_bytes = pay_count;
                msg_payload.sq = sq;

                total_sent += pay_count;
                pay_count = 0;

                bool sending = true;

                size_t seg_size = sizeof(segment_t);
                socklen_t addr_len = (socklen_t) sizeof(struct sockaddr_in);

                while (sending) {
                    bool corrupt = is_corrupted(loss_prob);
                    int cs = checksum(msg_payload.payload, corrupt);
                    msg_payload.checksum = cs;

                    snprintf(inf_msg_buf, INF_MSG_SIZE, "Sending segment with sq: %d, payload bytes: %zu, "
                                                        "checksum: %d", msg_payload.sq, msg_payload.payload_bytes,
                             msg_payload.checksum);
                    print_cmsg(inf_msg_buf);

                    ssize_t payload_bytes = sendto(sockfd, &msg_payload, seg_size, 0,
                                                   (struct sockaddr *) server, addr_len);

                    if (payload_bytes < 0) {
                        close(infd);
                        close(sockfd);
                        exit_cerr(__LINE__, "Sending Payload error");
                    } else if (!payload_bytes) {
                        sending = false;
                        print_cmsg("Ending Connection");

                    } else {
                        snprintf(inf_msg_buf, INF_MSG_SIZE, "Sent payload: \n%s", msg_payload.payload);
                        print_cmsg(inf_msg_buf);
                    }
                    print_sep();
                    print_sep();

                    memset(&ack_rec, 0, seg_size);

                    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
                        close(sockfd);
                        close(infd);
                        exit_cerr(__LINE__, "Error Setting timeout");
                    }
                    print_cmsg("Waiting for an ack");
                    ssize_t ack_bytes = recvfrom(sockfd, &ack_rec, seg_size, 0,
                                                 (struct sockaddr *) server, &addr_len);

                    if (ack_bytes < 0) {
                        print_cmsg("TIMEOUT reached resending ACK with new cs");

                    } else if (!ack_bytes) {
                        errno = ENOMSG;
                        close(sockfd);
                        close(infd);
                        exit_cerr(__LINE__, "Ending connection - no ACK received");
                    } else {
                        snprintf(inf_msg_buf, INF_MSG_SIZE, "ACK with sq: %d Received", ack_rec.sq);
                        print_cmsg(inf_msg_buf);

                        if (ack_rec.sq == sq) {
                            print_cmsg("SQ matches");
                            memset(payload, 0x00, PAYLOAD_SIZE);
                            sending = false;
                            sq++;
                            print_sep();
                            print_sep();
                        }
                    }
                }
            }
            payload[pay_count] = buff[i];
            pay_count++;
        }
    } else {
        errno = ENODATA;
        close(infd);
        close(sockfd);
        exit_cerr(__LINE__, "Failed to read file");
    }
    snprintf(inf_msg_buf, INF_MSG_SIZE, "Total segments sent: %d", sq);
    print_cmsg(inf_msg_buf);
    close(sockfd);
    close(infd);
    return total_sent;
}


