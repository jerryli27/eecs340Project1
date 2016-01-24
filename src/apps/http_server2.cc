#include "minet_socket.h"
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/stat.h>

#define BUFSIZE 1024
#define FILENAMESIZE 100
#define MAXFILESIZE 1024

int handle_connection(int);
int writenbytes(int,char *,int);
int readnbytes(int,char *,int);

int main(int argc,char *argv[])
{
  int server_port;
  int sock,sock2;
  struct sockaddr_in sa,sa2;
  int rc,i;
  fd_set readlist;
  fd_set connections;
  int maxfd;

  int backlog= 10; //Set backlog to whatever number of waiting connections you want in the server.

  /* parse command line args */
  if (argc != 3)
  {
    fprintf(stderr, "usage: http_server1 k|u port\n");
    exit(-1);
  }
  server_port = atoi(argv[2]);
  if (server_port < 1500)
  {
    fprintf(stderr,"INVALID PORT NUMBER: %d; can't be < 1500\n",server_port);
    exit(-1);
  }


  // COPIED FROM http_server1.cc; MAY NEED MODIFICATION LATER

  /* initialize and make socket */
  if (toupper(*(argv[1])) == 'K') { 
    minet_init(MINET_KERNEL);
  } else if (toupper(*(argv[1])) == 'U') { 
    minet_init(MINET_USER);
  } else {
    fprintf(stderr, "First argument must be k or u\n");
    exit(-1);
  }

  /* create socket */
  // Type must be SOCK_STREAM (TCP)
  // or SOCK_DGRAM (UDP)
  // or SOCK_ICMP (ICMP)
  sock=minet_socket (SOCK_STREAM);
  //sock2=minet_socket (SOCK_STREAM);
  

  /* set server address*/
  // Specify ip address (AF_INET) and port for struct sockaddr_in sa;
  sa.sin_family = AF_INET; //AF_INET means that src points to a character string containing an IPv4 network address in dotted-decimal format, "ddd.ddd.ddd.ddd"
  //Another option would be AF_INET6, which stands for ipv6 network
  sa.sin_port = htons(server_port);
  //set the address for sa. let the program automatically select an IP address:
  sa.sin_addr.s_addr = INADDR_ANY;

  //sa2.sin_family = AF_INET;
  //sa2.sin_port = htons(server_port);
  //sa2.sin_addr.s_addr = INADDR_ANY;


  /* bind listening socket */
  // The sockfd argument is a file descriptor that refers to a socket of
  // type SOCK_STREAM or SOCK_SEQPACKET.
  minet_bind (sock,&sa); 
  /**
   * TODO @@@@@@@@@@@ ATTENTION @@@@@@@@@@@
   * I think this one above should be 'sock', instead of 'SOCK_STREAM'
   * The following from minet_socket.cc:
   * @param[in] sockfd Handle to an unbound socket that was created
   * using minet_socket().
   */

  //minet_bind (sock2,&sa2);


  /* start listening */
  minet_listen(sock,backlog);
  //minet_listen(sock2,backlog2);

  // initialize the list of open connections to empty
  FD_ZERO(&connections);

  /* connection handling loop */
  while(1)
  {
    //i = minet_accept(sock,&sa);

    /* create read list */
    FD_COPY(&connections, &readlist);
    FD_SET(sock, &readlist);

    /* do a select */
    i = minet_select(sock, &readlist, NULL, NULL, NULL, NULL);  //timeout=NULL?

    /* process sockets that are ready */

      /* for the accept socket, add accepted connection to connections */
      if (i == sock)
      {
        sock2 = minet_accept(i, &sa);
        FD_SET(sock2, &connections);
      }
      else /* for a connection socket, handle the connection */
      {
	      rc = handle_connection(i);
        FD_CLR(i, &connections);
      }
  }
  minet_close(sock);

}




// COPIED FROM http_server1.cc; MAY NEED MODIFICATION LATER

int handle_connection(int sock2)
{
  char filecontent[MAXFILESIZE+1];
  char filename[FILENAMESIZE+1];

  FILE *fp;
  size_t pos = 0;
  int rc;
  int fd;
  struct stat filestat;
  char buf[BUFSIZE+1];
  char *headers;
  char *endheaders;
  char *bptr;
  int datalen=0;
  size_t addresslength;
  char *ok_response_f = "HTTP/1.0 200 OK\r\n"\
                      "Content-type: text/plain\r\n"\
                      "Content-length: %d \r\n\r\n";
  char ok_response[100];
  char *notok_response = "HTTP/1.0 404 FILE NOT FOUND\r\n"\
                         "Content-type: text/html\r\n\r\n"\
                         "<html><body bgColor=black text=white>\n"\
                         "<h2>404 FILE NOT FOUND</h2>\n"
                         "</body></html>\n";
  bool ok=true;



  /* first read loop -- get request and headers*/


  memset(buf, 0, sizeof(buf));

  //while((rc = minet_read(sock2, buf, BUFSIZE)) > 0){

    // header: after first \r\n
    // end of header: \r\n\r\n
    
    // The assumption was that if it can read one byte, 
    // it can read everything. So I'm going to set header
    // after this loop
  //}
  rc = minet_read(sock2, buf, BUFSIZE);


  // I'll use bptr to denote the start of file address
  bptr = buf + 5;
  headers = strstr(buf, "\r\n") + 2; // +2 because header is after this token
  endheaders = strstr(buf, "\r\n\r\n") + 4; // +4 so the end of headers is the end of string
  

  /* parse request to get file name */
  /* Assumption: this is a GET request and filename contains no spaces*/

  // char *tpl = "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n";
  // I assume that the first "%s" is the entirety of the file name
  // the first '/' will be skipped

  memset(filename, '\0', sizeof(filename));
  // memset(filecontent, '\0', sizeof(filecontent));

  addresslength = (int)(strstr(buf + 5, " ") - (buf + 5));
  memcpy(filename, buf + 5, addresslength);


    /* try opening the file */
  // we assume that this is not a directory request
  // therefore not checking S_ISDIR(filestat.st_mode)
  if ( stat( filename , &filestat ) < 0 ){
    ok = false;
  } else {

    fp = fopen( filename, "r" );
    
    int ich;
    while ( ( ich = getc( fp ) ) != EOF && pos < MAXFILESIZE) {
      filecontent[pos] = (char)ich;
      pos ++;
    }
    //filecontent[pos] = '\r';
    //filecontent[pos + 1] = '\n';
    //pos += 2;
  }

  /* send response */
  if (ok)
  {
    /* send headers */
    sprintf(ok_response,ok_response_f, pos - 1);
    char output[MAXFILESIZE+1];
    memset(output, '\0', sizeof(output));
    size_t tempPos = 0;
    size_t oksize = sizeof(ok_response);
    for (size_t i = 0; ok_response[i] != '\0' ; i ++ ) {
        output[tempPos] = ok_response[i];
        tempPos ++;
    }
    //output[tempPos] = '\r';
    //output[tempPos] = '\n';
    //tempPos += 2;
    for (size_t i = 0; i < pos ; i ++ ) {
        output[tempPos] = filecontent[i];
        tempPos ++;
    }

    //minet_write(sock2, ok_response, sizeof(ok_response));
    minet_write(sock2, output, tempPos);//ok_response, sizeof(ok_response));
    /* send file */
    //minet_write(sock2, filecontent, pos - 1);
  }
  else // send error response
  {
    minet_write(sock2, notok_response, strlen(notok_response));
  }

  /* close socket and free space */
  // no malloc or new; nothing to free or delete
  minet_close(sock2);


  if (ok)
    return 0;
  else
    return -1;
}






int readnbytes(int fd,char *buf,int size)
{
  int rc = 0;
  int totalread = 0;
  while ((rc = minet_read(fd,buf+totalread,size-totalread)) > 0)
    totalread += rc;

  if (rc < 0)
  {
    return -1;
  }
  else
    return totalread;
}

int writenbytes(int fd,char *str,int size)
{
  int rc = 0;
  int totalwritten =0;
  while ((rc = minet_write(fd,str+totalwritten,size-totalwritten)) > 0)
    totalwritten += rc;

  if (rc < 0)
    return -1;
  else
    return totalwritten;
}

