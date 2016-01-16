#include "minet_socket.h"
#include <stdlib.h>
#include <ctype.h>

#define BUFSIZE 1024

int write_n_bytes(int fd, char * buf, int count);
char *build_get_query(char *host, char *page);

int main(int argc, char * argv[]) {
    char * server_name = NULL;
    int server_port = 0;
    char * server_path = NULL;

    int sock = 0;
    int rc = -1; // I used this to store return values for each function.
    int datalen = 0;
    bool ok = true;
    struct sockaddr_in sa;
    FILE * wheretoprint = stdout;
    struct hostent * site = NULL;
    char * req = NULL;

    char buf[BUFSIZE + 1];
    char * bptr = NULL;
    char * bptr2 = NULL;
    char * endheaders = NULL;
    char * getQuery=NULL;
   
    struct timeval timeout;
    fd_set set;



    /*parse args */
    if (argc != 5) {
	fprintf(stderr, "usage: http_client k|u server port path\n");
	exit(-1);
    }

    server_name = argv[2];
    server_port = atoi(argv[3]);
    server_path = argv[4];



    /* initialize minet */
    if (toupper(*(argv[1])) == 'K') { 
	minet_init(MINET_KERNEL);
    } else if (toupper(*(argv[1])) == 'U') { 
	minet_init(MINET_USER);
    } else {
	fprintf(stderr, "First argument must be k or u\n");
	exit(-1);
    }

    /* create socket */
    // Create a socket.  Type must be SOCK_STREAM (TCP)
    // or SOCK_DGRAM (UDP)
    // or SOCK_ICMP (ICMP)
    sock=minet_socket (SOCK_STREAM);
    // Do DNS lookup
    /* Hint: use gethostbyname() */
    struct hostent *hostentry;
    if ((hostentry = gethostbyname(server_name)) == NULL) {  // get the host info
        fprintf(stderr, "Failed to get host ip address at: %s\n",server_name);
        exit(-1);
    }

    /* set address */
    // Specify ip address (AF_INET) and port for struct sockaddr_in sa;
    sa.sin_family = AF_INET; //AF_INET means that src points to a character string containing an IPv4 network address in dotted-decimal format, "ddd.ddd.ddd.ddd"
    //Another option would be AF_INET6, which stands for ipv6 network
    sa.sin_port = htons(server_port);
    //set the address for sa.
    if (inet_pton(AF_INET,hostentry->h_addr, &sa.sin_addr.s_addr)<=0){
        fprintf(stderr, "Can't set remote->sin_addr.s_addr. %s is not a valid IP address\n",hostentry->h_addr);
        exit(-1);
    }
    /* connect socket */

    // Connect to a remote socket.
    // the IP address (AF_INET) and port are specified in myaddr.
    // The sockfd argument is a file descriptor that refers to a socket of
    // type SOCK_STREAM or SOCK_SEQPACKET.
    minet_bind (SOCK_STREAM,sa);
    if(minet_connect(sock, sa) < 0){
        fprintf("Could not connect to socket");
        exit(-1);
    }
    /* send request */
    // Write to a connected socket (returns # bytes actually written).
    getQuery=build_get_query(server_name,server_path);
    write_n_bytes(sock, getQuery, strlen(getQuery));
  

    /* wait till socket can be read */
    /* Hint: use select(), and ignore timeout for now. */

    
    /* first read loop -- read headers */
    memset(buf, 0, sizeof(buf));
      int htmlstart = 0;
      char * htmlcontent;
    while((rc = recv(sock, buf, BUFSIZ, 0)) > 0){
    if(htmlstart == 0)
    {
      /* Under certain conditions this will not work.
      * If the \r\n\r\n part is splitted into two messages
      * it will fail to detect the beginning of HTML content
      */
      htmlcontent = strstr(buf, "\r\n\r\n");
      if(htmlcontent != NULL){
        htmlstart = 1;
        htmlcontent += 4;
      }
    }else{
      htmlcontent = buf;
    }
    if(htmlstart){
      fprintf(stdout, htmlcontent);
    }

        memset(buf, 0, rc);
    }
    if(rc < 0)
    {
    perror("Error receiving data");
    }
    /* examine return code */   
    //Skip "HTTP/1.0"
    //remove the '\0'
    // Normal reply has return code 200

    /* print first part of response */

    /* second read loop -- print out the rest of the response */
    
    /*close socket and deinitialize */
    free(getQuery);
    minet_close(sock);

    if (ok) {
	return 0;
    } else {
	return -1;
    }
}

int write_n_bytes(int fd, char * buf, int count) {
    int rc = 0;
    int totalwritten = 0;

    while ((rc = minet_write(fd, buf + totalwritten, count - totalwritten)) > 0) {
	totalwritten += rc;
    }
    
    if (rc < 0) {
	return -1;
    } else {
	return totalwritten;
    }
}


char *build_get_query(char *host, char *page){
    // Useful little function
    // modified from http://coding.debuntu.org/c-linux-socket-programming-tcp-simple-http-client
    char* query;
    char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: %s\r\n\r\n";
    // -5 is to consider the %s %s %s in tpl and the ending \0
    query = (char *)malloc(strlen(host)+strlen(page)+strlen(USERAGENT)+strlen(tpl)-5);
    sprintf(query, tpl, page, host, USERAGENT);
    return query;
}