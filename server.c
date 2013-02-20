#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#define MAX_SALES 2000
typedef struct {
	char seller[300], item[300], price[300],buyer[300],formatted[3000];
	int time;
} sale;
char *mcAdd = "224.0.240.240";
int sd, rc, n, cliLen, ud, connfd, runningSales = 0;
struct ip_mreq mreq;
struct sockaddr_in cliAddr, addr, server_addr,cliaddr;
struct in_addr mcastAddr;
struct hostent *h;
sale currentSales[MAX_SALES]; 
char msg[3000];

void setup() {
	if ((sd=socket(AF_INET,SOCK_DGRAM,0)) < 0) exit(EXIT_FAILURE);
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(mcAdd);
	addr.sin_port=htons(1500);
	ud = socket(AF_INET, SOCK_STREAM, 0);
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	server_addr.sin_port = htons(1600);
	bind(ud, (struct sockaddr*) &server_addr, sizeof(server_addr));
}

void sendMulti(char *out) {
	int r = sendto(sd, out, strlen(out)+1, 0, (struct sockaddr *) &addr, sizeof(addr));
}

sale splitSale(char *in) {
	sale s;
	char *split = NULL;
	char str[3000];
	strcpy(str, in);
	split = strtok(str, "&");
	split = strtok(NULL, "&");
	strcpy(s.seller, split);
	strcpy(s.buyer, split);
	split = strtok(NULL, "&");
	strcpy(s.item, split);
	split = strtok(NULL, "&");
	strcpy(s.price, split);
	split = strtok(NULL, "&");
	s.time = time(NULL) + strtol(split,NULL,0L );
	sprintf(s.formatted, "SALE&%s&%s&%s&%s&%d&DST", s.seller, s.item, s.price, s.buyer, s.time);
	return s;
}

void recieveBid(char *in) {
	printf("Bid Recieved: %s\n", in);
	char* split;
	char item[600], bidder[300];
	float bid, oldBid;
	split = strtok(in, "&");
	split = strtok(NULL, "&");
	strcpy(item, split);
	split = strtok(NULL, "&");
	bid = strtof(split, NULL);
	split = strtok(NULL, "&");
	strcpy(bidder, split);
	int i;
	for(i = 0; i <= runningSales; i++) {
		if((strncmp(currentSales[i].item, item, strlen(item))) == 0) {
			if((oldBid = strtof(currentSales[i].price, NULL)) < bid) {
				sprintf(currentSales[i].price, "%2.2f", bid);
				sprintf(currentSales[i].buyer, "%s", bidder);
				sprintf(currentSales[i].formatted, "SALE&%s&%s&%s&%s&%d&DST", currentSales[i].seller, currentSales[i].item, currentSales[i].price,currentSales[i].buyer, currentSales[i].time);
			}
		}
	}
}

void recieveMsg(char *in) {
	if((strncmp(in, "SALE&", 5)) != 0) recieveBid(in);
	else {
	printf("Sale Recieved: %s\n", in);
		sale s = splitSale((char*)in);
		int i = firstFree();
		currentSales[i] = s;
		sendMulti(s.formatted);
		runningSales++;
	}
}

int firstFree() {
	int i;for(i = 0; i < MAX_SALES; i++) if(strlen(currentSales[i].formatted) < 10) return i;return -1;}

void alertAllSales() {
	int i;
	for(i = 0; i <= runningSales;i++) {
		if(strlen(currentSales[i].formatted) > 8) {
			if(currentSales[i].time > time(NULL)) sendMulti(currentSales[i].formatted);
			else {
				printf("Sale ended: %s\n", currentSales[i].formatted);
				sprintf(currentSales[i].formatted, "END&%s&%s&%s&%s&%d&DST", currentSales[i].seller, currentSales[i].item, currentSales[i].price, currentSales[i].buyer, currentSales[i].time);
				sendMulti(currentSales[i].formatted);
				sale p;
				currentSales[i] = p;
				runningSales--;
			}
		}
	}
	signal(SIGALRM, alertAllSales);
	alarm(5);
}
int main(int argc, char *argv[]) {
	setup(); alertAllSales();
	struct sockaddr_in peer_addr;
	socklen_t peer_socklen;
	listen(ud, 1024);
	while(1) {
		int cfd = accept(ud, (struct sockaddr*) &peer_addr, (socklen_t*) &peer_socklen);
		int nr = read(cfd, msg, 3000);
		if(nr > 0) { msg[nr] = '\0';recieveMsg(msg);}
	}
	return 0;
}
