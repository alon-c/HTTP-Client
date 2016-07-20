#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#define noneFlag -1
#define defPort 80
#define userAgent "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

void usage();

int isDigits(char *str);

int main(int argc, char **argv) {
	if ((argc > 5) || (argc <= 1)) {
		usage();
		exit(-1);
	}

	// check if args are valid and set the locations
	int hLoc = noneFlag, dLoc = noneFlag, URLLoc = noneFlag;
	int argCount = 0; // count the valid args
	int i = 1;
	while (i < argc) {
		if (strcmp(argv[i], "-h") == 0) {
			if (hLoc == noneFlag) {
				hLoc = i;
				argCount++;
			}
			else {
				usage();
				exit(-1);
			}
		}
		else if (strcmp(argv[i], "-d") == 0) {
			if (i == (argc -1)) {
				usage();
				exit(-1);
			}
			if (dLoc == noneFlag) {
				dLoc = i;
				argCount += 2;
				i += 2;
				continue;
			}
			else {
				usage();
				exit(-1);
			}
		}
		else { // must be the URL
			if (URLLoc == noneFlag) {
				URLLoc = i;
				argCount++;
			}
			else {
				usage();
				exit(-1);
			}
		}
		i++;
	}

	if ((URLLoc == noneFlag) || (argCount != (argc -1))) {
		usage();
		exit(-1);
	}

	//parse the URL
	const int maxLen = strlen(argv[URLLoc]) +1;
	char portStr[maxLen], host[maxLen], filepath[maxLen];
	int port = noneFlag;
	int isPort = noneFlag;

	//check the protocol from the URL
	if (!strstr(argv[URLLoc], "http://")) {
		fprintf(stderr, "wrong input\r\n");
		exit(-1);
	}
	
	int j = 7; // start parsing afther the protocol
int id = 0;
	while (argv[URLLoc][j]) {
		if (argv[URLLoc][j] == ':') {
			j++; // point to next then ':'
			isPort = 0;
			break;
		}
		if (argv[URLLoc][j] == '/') {
			break;
		}
		host[id] = argv[URLLoc][j];
		id++;
		j++;
	}
	host[id] = '\0';
	strcpy(host, host);
	if (strlen(host) == 0) {
		fprintf(stderr, "wrong input\r\n");
		exit(-1);
	}

	if (isPort != noneFlag) {
		id = 0;
	while (argv[URLLoc][j]) {
			if (argv[URLLoc][j] == '/') {
				break;
			}
			portStr[id] = argv[URLLoc][j];
			id++;
			j++;
		}
		portStr[id] = '\0';
		strcpy(portStr, portStr);
		if ((sscanf(portStr, "%d", &port) != 1) || (isDigits(portStr) == -1) || (strlen(portStr) == 0)) {
			fprintf(stderr, "wrong input\r\n");
			exit(-1);
		}
	}
	else { // the URL did not contain port
		port = defPort;
	}

	id = 0;
	while (argv[URLLoc][j]) {
		filepath[id] = argv[URLLoc][j];
		id++;
		j++;
	}
	filepath[id] = '\0';
	strcpy(filepath, filepath);
	if (strlen(filepath) == 0) {
		fprintf(stderr, "wrong input\r\n");
		exit(-1);
	}

	// if -d flag exists, then parse the time
		char timebuf[128]; 
	if (dLoc != noneFlag) {
		const int dateLen = strlen(argv[(dLoc +1)]) +1;
		char dayStr[dateLen], hourStr[dateLen], minStr[dateLen];
		int day = noneFlag, hour = noneFlag, min = noneFlag; // time holders
		id = 0;
		j = 0;
		while (argv[(dLoc +1)][j]) {
			if (argv[(dLoc +1)][j] == ':') {
				j++;
				break;
			}
			dayStr[id] = argv[(dLoc+1)][j];
			id++;
			j++;
		}
		dayStr[id] = '\0';
		strcpy(dayStr, dayStr);
		if ((sscanf(dayStr, "%d", &day) != 1) || (isDigits(dayStr) == -1) || (strlen(dayStr) == 0)) {
			fprintf(stderr, "wrong input\r\n");
			exit(-1);
		}

		id = 0;
		while (argv[(dLoc +1)][j]) {
			if (argv[(dLoc +1)][j] == ':') {
				j++;
				break;
			}
			hourStr[id] = argv[(dLoc +1)][j];
			id++;
			j++;
		}
		hourStr[id] = '\0';
		strcpy(hourStr, hourStr);
		if ((sscanf(hourStr, "%d", &hour) != 1) || (isDigits(hourStr) == -1) || (strlen(hourStr) == 0)) {
			fprintf(stderr, "wrong input\r\n");
			exit(-1);
		}

		id = 0;
		while (argv[(dLoc +1)][j]) {
			minStr[id] = argv[(dLoc+1)][j];
			id++;
			j++;
		}
		minStr[id] = '\0';
		strcpy(minStr, minStr);
		if ((sscanf(minStr, "%d", &min) != 1) || (isDigits(minStr) == -1) || (strlen(minStr) == 0)) {
			fprintf(stderr, "wrong input\r\n");
			exit(-1);
		}

		//convert time 
		time_t now; 
		now = time(NULL); 
		now=now-(day*24*3600+hour*3600+min*60); //where day, hour and min are the values from the input 
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now)); //timebuf holds the correct format of the time.
	}

	// create TCP socket
	int sd;
	if ((sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		exit(-1);
	}

	struct hostent *hent;
	char *ip = (char*) malloc(sizeof(char) * (INET_ADDRSTRLEN));
	if (!ip) {
		perror("malloc");
			exit(-1);
	}

	memset(ip, 0, INET_ADDRSTRLEN);
if((hent = gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		exit(-1);
	}
	if ((inet_ntop(AF_INET, (void *) (hent->h_addr_list[0]), ip, INET_ADDRSTRLEN)) == NULL) {
		perror ("inet_ntop");
		exit(-1);
	}
	struct sockaddr_in *remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
	if(!remote) {
		perror("malloc");
		exit(-1);
	}

	remote->sin_family = AF_INET;
	if (inet_pton(AF_INET, ip, (void*) (&(remote->sin_addr.s_addr))) <= 0) {
		perror("inet_pton");
		exit(-1);
	}
	remote->sin_port = htons(port);

	// connect to the server
	if(connect(sd, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0) {
		perror("connect");
		exit(-1);
	}

	// create request
			const char *requestHeader;
	char *modifiedHeader;
	if (hLoc != noneFlag) {
		requestHeader = "HEAD %s%s HTTP/1.0\r\n\r\n"; // format host and filepath
		modifiedHeader = "";
	}
else {
		if (dLoc == noneFlag) {
			requestHeader = "GET %s%s HTTP/1.0\r\n\r\n"; // format host and filepath
			modifiedHeader = "";
		}
		else { // the request shuld contain if-modified-since field
						requestHeader = "GET %s%s HTTP/1.0\r\nIf-Modified-Since: "; // format host and filepath
			modifiedHeader = "";
		char modifiedHeaderTmp[(strlen(timebuf) + strlen(modifiedHeader) + strlen("\r\n\r\n") + strlen(requestHeader) +1)];
			strcpy(modifiedHeaderTmp, requestHeader);
			strcat(modifiedHeaderTmp, timebuf);
			strcat(modifiedHeaderTmp, "\r\n\r\n");
			modifiedHeader = modifiedHeaderTmp;
				requestHeader = modifiedHeader;
		}
	}

		const int requestLen = strlen(host) + strlen(filepath) + strlen(requestHeader) -4;
		char request[(requestLen +1)];
		if (sprintf(request, requestHeader, host, filepath) != requestLen) {
			perror("sprintf");
			exit(-1);
		}

	printf("HTTP request =\r\n%s\r\nLEN = %d\r\n",request, requestLen);

	// send request to server
	int sent = 0, sentret = 0;
	while (sent < strlen(request)) {
		sentret = send(sd, request +sent, strlen(request) -sent, 0);
		if (sentret == 0) {
			perror("send");
			exit(-1);
		}
		sent += sentret;
	}

	// receive response from server
	char buf[BUFSIZ+1];
	memset(buf, 0, sizeof(buf));
	int totalBytes = 0;
	while ((sentret = recv(sd, buf, BUFSIZ, 0)) > 0) {
		if (sentret > 0) {
			fprintf(stdout, "%s", buf);
			memset(buf, 0, sentret);
			totalBytes += sentret;
		}
	}
		if (sentret < 0) {
			perror("recv");
		}

	printf("\r\n Total received response bytes: %d\r\n",totalBytes);

	free(remote);
	free(ip);
	close(sd);

	return 0;
}

void usage() {
printf("Usage: client [-h] [-d <timeinterval>] <URL>\r\n");
}

/*
** check if the a string holds digits only
** returns 0 for only digits and -1 in case of a failure
*/
int isDigits(char *str) {
	int i = 0;
while (str[i]) {
		if (isdigit(str[i]) == 0) {
			return -1;
		}
		i++;
	}

return 0;
}