/* aprs.fi.toudp.c v1.0 by  james@ustc.edu.cn 2015.12.19

   connect to china.aprs2.net. tcp 14580 port, login filter p/B
   send all packets to udp host in file "/usr/src/arps/aprs.fi.udpdest

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include "sock.h"
#include <ctype.h>

#define MAXLEN 16384

// #define DEBUG 1

#define PORT 14580

int r_fd;

void sendudp(char*buf, int len, char *host, int port)
{
        struct sockaddr_in si_other;
        int s, slen=sizeof(si_other);
        int l;
#ifdef DEBUG
        fprintf(stderr,"send to %s,",host);
#endif
        if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))==-1) {
                fprintf(stderr,"socket error");
                return;
        }
        memset((char *) &si_other, 0, sizeof(si_other));
        si_other.sin_family = AF_INET;
        si_other.sin_port = htons(port);
        if (inet_aton(host, &si_other.sin_addr)==0) {
                fprintf(stderr, "inet_aton() failed\n");
                close(s);
                return;
        }
        l = sendto(s, buf, len, 0, (const struct sockaddr *)&si_other, slen);
#ifdef DEBUG
        fprintf(stderr,"%d\n",l);
#endif
        close(s);
}

void relayaprs(char *buf, int len)
{
        FILE *fp;
        fp = fopen("/usr/src/aprs/aprs.fi.udpdest","r");
        if (fp==NULL) {
                fprintf(stderr, "open host error\n");
                return;
        }
        char hbuf[MAXLEN];
        while(fgets(hbuf,MAXLEN,fp)) {
                char *p;
                if(strlen(hbuf)<5) continue;
                if(hbuf[strlen(hbuf)-1]=='\n')
                        hbuf[strlen(hbuf)-1]=0;
                p = strchr(hbuf,':');
                if(p) {
                        *p=0;
                        p++;
                        sendudp(buf,len,hbuf,atoi(p));
                } else
                        sendudp(buf,len,hbuf,PORT);
        }
        fclose(fp);
}

#include "passcode.c"

void Process(char *call) 
{	
	char buffer[MAXLEN];
	int n;

	r_fd= Tcp_connect("china.aprs2.net","14580");
	snprintf(buffer,MAXLEN,"user %s pass %d vers fwd 1.5 filter p/B\r\n",call,passcode(call));
	Write(r_fd, buffer, strlen(buffer));

	while (1) {
		n = Readline(r_fd, buffer, MAXLEN);
		if(n<=0)   {
			exit(0);
		}
		if(buffer[0]=='#') continue;
		buffer[n]=0;
		relayaprs(buffer,n);
	}
}

void usage()
{
	printf("\naprstcp v1.0 - aprs relay by james@ustc.edu.cn\n");
	printf("\naprstcp x.x.x.x 14580 china.aprs2.net 14580\n\n");
	exit(0);
}

int main(int argc, char *argv[])
{
	char *call="BG6CQ-5";
	signal(SIGCHLD,SIG_IGN);
	if(argc>1) call=argv[1];

#ifndef DEBUG
	daemon_init("aprsrelay",LOG_DAEMON);
	while(1) {
                int pid;
                pid=fork();
                if(pid==0) // i am child, will do the job
                        break;
                else if(pid==-1) // error
                        exit(0);
                else
                        wait(NULL); // i am parent, wait for child
                sleep(2);  // if child exit, wait 2 second, and rerun
        }
#endif

	Process(call);
	return(0);
}
