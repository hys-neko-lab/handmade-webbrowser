#include "client.h"

int get_http(char *dest, int port)
{
	int n;
	int err;
	int sock;
	struct sockaddr_in serv;
	struct hostent *host;
	char buf[CLIENT_BUF_SIZE];
	unsigned int **p_addr;
	FILE *fp;

	/* Prepare socket */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) return 1;
	
	/* Set sockaddr_info */
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = inet_addr(dest);

	/* Connect to server */
	if (serv.sin_addr.s_addr == 0xFFFFFFFF){
		host = gethostbyname(dest);
		if (host == NULL) return 1;

		/* Pointer to host network information */
		p_addr = (unsigned int **)host->h_addr_list;
		while(*p_addr != NULL){
			serv.sin_addr.s_addr = *(*p_addr);
			err = connect(
				sock, 
				(struct sockaddr *)&serv, 
				sizeof(serv));
			if (0 == err) break;
			p_addr++;
		}
		if (*p_addr == NULL){
			perror("connect");
			return 1;
		}
	} else{
		err = connect(
			sock, 
			(struct sockaddr *)&serv, 
			sizeof(serv));
		if (0 != err){
			perror("connect");
			return 1;
		}
	}

	/* Send HTTP request */
	memset(buf, 0, sizeof(buf));
	snprintf(buf, sizeof(buf),
		"GET / HTTP/1.0\r\n\r\n");
	n = write(sock, buf, (int)strlen(buf));
	if (n < 0){
		perror("write");
		return 1;
	}

	/* Prepare catche file */
	if ((fp = fopen("catche", "w")) == NULL){
		perror("fopen");
		return 1;
	}

	/* Receive HTTP response */
	while (n > 0){
		memset(buf, 0, sizeof(buf));
		n = read(sock, buf, sizeof(buf));
		if (n < 0){
			perror("read");
			return 1;
		}
		fputs(buf, fp);
	}

	fclose(fp);
	close(sock);

	return 0;
}

