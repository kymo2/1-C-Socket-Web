/*
 * tiny_client.c — fetch “/” from an HTTP server and dump the response.
 *
 * build : gcc -Wall -Wextra -pedantic -std=c99 -o tiny_client tiny_client.c
 * usage : ./tiny_client <IPv4-address>
 */

#include <sys/socket.h>    // socket(), connect(), read(), write()
#include <netinet/in.h>    // sockaddr_in, htons()
#include <arpa/inet.h>     // inet_pton()
#include <unistd.h>        // close()
#include <errno.h>
#include <stdarg.h>        // va_list, va_start, va_end
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SERVER_PORT 80      // default HTTP port
#define MAXLINE     4096    // buffer size
#define SA          struct sockaddr

/*--------------------------------------------------------------*/
static void err_n_die(const char *fmt, ...);
/*--------------------------------------------------------------*/

int main(int argc, char **argv)
{
    /* ---------- 0. sanity check ------------------------------------ */
    if (argc != 2)
        err_n_die("usage: %s <server IPv4 address>", argv[0]);

    /* ---------- 1. create socket ----------------------------------- */
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);   // TCP/IPv4
    if (sockfd < 0)
        err_n_die("socket");

    /* ---------- 2. build server address ---------------------------- */
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(SERVER_PORT);       // host-to-network

    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        err_n_die("inet_pton: %s", argv[1]);

    /* ---------- 3. connect ----------------------------------------- */
    if (connect(sockfd, (SA *)&servaddr, sizeof servaddr) < 0)
        err_n_die("connect");

    /* ---------- 4. compose HTTP request ---------------------------- */
    char sendline[MAXLINE];
    int  sendbytes = snprintf(sendline, sizeof sendline,
                              "GET / HTTP/1.1\r\n"
                              "Host: %s\r\n"
                              "Connection: close\r\n\r\n",
                              argv[1]);
    if (sendbytes < 0 || sendbytes >= MAXLINE)
        err_n_die("request too large");

    /* ---------- 5. send request ------------------------------------ */
    if (write(sockfd, sendline, sendbytes) != sendbytes)
        err_n_die("write");

    /* ---------- 6. read & print response --------------------------- */
    char recvline[MAXLINE];
    ssize_t n;
    while ((n = read(sockfd, recvline, MAXLINE - 1)) > 0) {
        recvline[n] = '\0';          // null-terminate chunk
        fputs(recvline, stdout);
    }
    if (n < 0)
        err_n_die("read");

    /* ---------- 7. cleanup ----------------------------------------- */
    close(sockfd);
    return 0;
}

/*--------------------------------------------------------------*/
static void err_n_die(const char *fmt, ...)
{
    int  errno_save = errno;         // preserve errno
    va_list ap;
    va_start(ap, fmt);

    vfprintf(stderr, fmt, ap);
    va_end(ap);

    if (errno_save != 0)
        fprintf(stderr, "  (errno %d: %s)", errno_save, strerror(errno_save));

    fputc('\n', stderr);
    exit(EXIT_FAILURE);
}
