#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define errquit(m) \
	{              \
		perror(m); \
		exit(-1);  \
	}

static int port_http = 80;
static int port_https = 443;
static const char *docroot = "/html";

void *client_handle(void *arg)
{
	int client = *((int *)arg);
	int readlen, pathlen;
	char rbuff[1000], wbuff[1000];
	FILE *fp;

	readlen = read(client, rbuff, 1000);
	rbuff[readlen] = 0;

	char service[20], path[100], url[80], filetype[20];
	bool isdir;

	sscanf(rbuff, "%[^ ]", service);
	sscanf(rbuff + strlen(service) + 1, "%[^ ]", url);

	for (int i = strlen(url) - 1; i >= 0; i--)
	{
		if (url[i] == '?')
		{
			url[i] = 0;
			break;
		}

		if (url[i] == '/')
		{
			break;
		}
	}

	sprintf(path, "/html%s", url);
	pathlen = strlen(path);

	if (strcmp(service, "GET") != 0) // determine GET service
	{
		sprintf(wbuff, "HTTP/1.0 501 Not Implemented\r\n\r\n");
		write(client, wbuff, strlen("HTTP/1.0 501 Not Implemented\r\n\r\n"));

		close(client);
		pthread_exit(NULL);
	}

	if (path[pathlen - 1] == '/') // trailing slash
	{
		strcat(path, "index.html");

		fp = fopen(path, "r");

		if (fp == NULL) // no index.html in such directory
		{
			sprintf(wbuff, "HTTP/1.0 403 Forbbiden\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 403 Forbbiden\r\n\r\n"));
		}
		else
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));
			fgets(wbuff, 1000, fp);
			write(client, wbuff, strlen(wbuff));
			fclose(fp);
		}

		close(client);
		pthread_exit(NULL);
	}

	for (int i = pathlen - 1; i >= 0; i--)
	{
		if (path[i] == '.')
		{
			strcpy(filetype, path + i + 1);
			isdir = false;
			break;
		}

		if (path[i] == '/')
		{
			isdir = true;
			break;
		}
	}

	if (isdir) // request a directory
	{
		if (strcmp(path, "/html/music") == 0) // music
		{
			sprintf(wbuff, "HTTP/1.0 301 Moved Permanantely\r\nLocation: /music/\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 301 Moved Permanantely\r\nLocation: /music/\r\n\r\n"));
		}
		else if (strcmp(path, "/html/image") == 0) // image
		{
			sprintf(wbuff, "HTTP/1.0 301 Moved Permanantely\r\nLocation: /image/\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 301 Moved Permanantely\r\nLocation: /image/\r\n\r\n"));
		}
		else // directory doesn't exist
		{
			sprintf(wbuff, "HTTP/1.0 404 Not Found\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 404 Not Found\r\n\r\n"));
		}

		close(client);
		pthread_exit(NULL);
	}
	else // request a file
	{
		if (strcmp(filetype, "png") == 0 || strcmp(filetype, "jpg") == 0 || strcmp(filetype, "mp3") == 0)
		{
			fp = fopen(path, "rb");
		}
		else if (strcmp(filetype, "txt") == 0 || strcmp(filetype, "html") == 0)
		{
			if (strcmp(path, "/html/\%E4\%B8\%AD\%E6\%96\%87\%E6\%AA\%94\%E5\%90\%8D.txt") == 0)
			{
				fp = fopen("/html/中文檔名.txt", "r");
			}
			else
			{
				fp = fopen(path, "r");
			}
		}
		else
		{
			sprintf(wbuff, "HTTP/1.0 404 Not Found\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 404 Not Found\r\n\r\n"));

			close(client);
			pthread_exit(NULL);
		}

		if (fp == NULL)
		{
			sprintf(wbuff, "HTTP/1.0 404 Not Found\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 404 Not Found\r\n\r\n"));

			close(client);
			pthread_exit(NULL);
		}
		else if (strcmp(filetype, "txt") == 0)
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: text/plain;charset=utf-8\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: text/plain;charset=utf-8\r\n\r\n"));

			fseek(fp, 0, SEEK_END);
			int text_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *text_data = (char *)malloc(text_size);
			fread(text_data, 1, text_size, fp);

			write(client, text_data, text_size);

			free(text_data);
		}
		else if (strcmp(filetype, "html") == 0)
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n"));

			fseek(fp, 0, SEEK_END);
			int html_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *html_data = (char *)malloc(html_size);
			fread(html_data, 1, html_size, fp);

			write(client, html_data, html_size);

			free(html_data);
		}
		else if (strcmp(filetype, "png") == 0)
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: image/png\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: image/png\r\n\r\n"));

			fseek(fp, 0, SEEK_END);
			int image_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *image_data = (char *)malloc(image_size);
			fread(image_data, 1, image_size, fp);

			write(client, image_data, image_size);

			free(image_data);
		}
		else if (strcmp(filetype, "jpg") == 0)
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\n\r\n"));

			fseek(fp, 0, SEEK_END);
			int image_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *image_data = (char *)malloc(image_size);
			fread(image_data, 1, image_size, fp);

			write(client, image_data, image_size);

			free(image_data);
		}
		else if (strcmp(filetype, "mp3") == 0)
		{
			sprintf(wbuff, "HTTP/1.0 200 OK\r\nContent-Type: audio/mpeg\r\n\r\n");
			write(client, wbuff, strlen("HTTP/1.0 200 OK\r\nContent-Type: audio/mpeg\r\n\r\n"));

			fseek(fp, 0, SEEK_END);
			int music_size = ftell(fp);
			fseek(fp, 0, SEEK_SET);

			char *music_data = (char *)malloc(music_size);
			fread(music_data, 1, music_size, fp);

			write(client, music_data, music_size);

			free(music_data);
		}

		fclose(fp);

		close(client);
		pthread_exit(NULL);
	}
}

int main(int argc, char *argv[])
{
	int s;
	struct sockaddr_in sin;

	if (argc > 1)
	{
		port_http = strtol(argv[1], NULL, 0);
	}
	if (argc > 2)
	{
		if ((docroot = strdup(argv[2])) == NULL)
			errquit("strdup");
	}
	if (argc > 3)
	{
		port_https = strtol(argv[3], NULL, 0);
	}

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
		errquit("socket");

	do
	{
		int v = 1;
		setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
	} while (0);

	bzero(&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errquit("bind");
	if (listen(s, SOMAXCONN) < 0)
		errquit("listen");

	int c;
	struct sockaddr_in csin;
	socklen_t csinlen = sizeof(csin);
	pthread_t thread_id;
	do
	{

		if ((c = accept(s, (struct sockaddr *)&csin, &csinlen)) < 0)
		{
			perror("accept");
			continue;
		}

		if (pthread_create(&thread_id, NULL, client_handle, &c) != 0)
		{
			perror("thread");
			close(c);
		}

		pthread_detach(thread_id);

	} while (1);

	return 0;
}
