#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define MULTI_PORT 1500
#define MULTI_GROUP "224.0.240.240"
#define MSGBUFSIZE 2000
#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 1600
#define MAX_SALES 1024
typedef struct {
	char seller[300], item[300], price[50],buyer[300], formated[3000];
	int time;
} sale;
char user[255];

sale splitSale(char *in) {
	sale s; char *split;
	split = strtok(in, "&");
	split = strtok(NULL, "&");
	strcpy(s.seller, split);
	split = strtok(NULL, "&");
	strcpy(s.item, split);
	split = strtok(NULL, "&");
	strcpy(s.price, split);
	split = strtok(NULL, "&");
	strcpy(s.buyer, split);
	split = strtok(NULL, "&");
	s.time = time(NULL) + strtol(split,NULL,0L );
	sprintf(s.formated, "SALE&%s&%s&%s&%s&%d&DST", s.seller, s.item, s.price,s.buyer, s.time);
	return s;
}

void printSale(char *in) {
	sale s = splitSale(in);
	int comp = strncmp(in, "SALE", 4);
	if(comp == 0) printf("Seller: %s\nItem: %s\nPrice: $%s\n", s.seller, s.item, s.price);
	else printf("The bidding for %s has closed with a final price of $%s, bought by %s\n", s.item, s.price, s.buyer);
}

void sendTCP(char *out) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(1600);
	inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
	int con = connect(fd, (struct sockaddr*) &server_addr, sizeof(server_addr));
	if(con == 0) {
		int ns = write(fd, out, strlen(out));
		close(fd);
	}
}

void makeBid() {
	char item[255], price[50], out[400];
	gets(out);
	printf("Enter the item you want to bid on. Case is important: ");
	gets(item);
	printf("Enter a new price, in the format 0.00: ");
	gets(price);	
	sprintf(out, "BID&%s&%s&%s", item, price, user);
	sendTCP(out);
}

void makeSale() {
	char item[255], price[50], lenn[10], out[400];
	gets(out);
	printf("Enter the item you want to sell. Case is important: ");
	gets(item);
	printf("Enter price, in the format 0.00: ");
	gets(price);
	printf("Enter the number of minutes you want the auction to run for: ");
	gets(lenn);
	int slen = strtol(lenn, NULL, 10);
	slen = slen * 60;
	sprintf(out, "SALE&%s&%s&%s&%d&DST", user, item, price, slen);
	sendTCP(out);
}

void interuptOps() {
	printf("Menu Options\n======================\n");
	printf("Press 1 to enter sale\nPress 2 to enter bid\nPress 9 to quit\n");
	char in[2];
	fgets(in, 2, stdin);
	int choice = strtol(in, NULL, 10);
	switch(choice) {
		case 9:
			exit(0);
		case 2:
			makeBid();
			break;
		case 1:
			makeSale();
			break;
		default: return;
	}
}

main(int argc, char *argv[]) {
	printf("Welcome, user. Please enter your name: ");
	gets(user);
	int sockfd,n,fd, nbytes,addrlen;
	struct sockaddr_in servaddr,cliaddr,addr;
	char sendline[1000], recvline[1000], msgbuf[MSGBUFSIZE];
	struct ip_mreq mreq;
	u_int yes=1;
	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {perror("socket");exit(1);}
	if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes)) < 0){ perror("Reusing ADDR failed");}
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(MULTI_PORT);
	if (bind(fd,(struct sockaddr *) &addr,sizeof(addr)) < 0){ perror("bind");}
	mreq.imr_multiaddr.s_addr=inet_addr(MULTI_GROUP);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0){ perror("setsockopt");}
	signal(SIGINT, interuptOps);
	while (1) {
		addrlen=sizeof(addr);
		if ((nbytes=recvfrom(fd,msgbuf,MSGBUFSIZE,0,(struct sockaddr *)&addr, &addrlen)) < 0) exit(1);
		printSale(msgbuf);
	}
}
