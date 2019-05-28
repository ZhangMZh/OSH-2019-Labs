#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MAX_THREAD 50
#define MAX_QUEUE  1024
#define TRUE       1
#define FALSE      0

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 1024

#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_404 "404 Not Found"
#define HTTP_STATUS_500 "500 Internal Server Error"

typedef struct
{
    pthread_mutex_t lock;
    pthread_cond_t queue_not_full;
    pthread_cond_t queue_not_empty;

    pthread_t *threads;
    int thread_num;

    int *queue;
    int queue_num;
    int head;
    int tail;
}threadpool_t;

threadpool_t *threadpool_create(int thread_num, int queue_max_num);
void *threadpool_thread(void *threadpool);
int threadpool_addtask(threadpool_t *pool, int clnt_sock);

int parse_request(char* request, ssize_t req_len, char *filepath);
void handle_clnt(int clnt_sock);

threadpool_t *threadpool_create(int thread_num, int queue_max_num)
{
    int i;
    threadpool_t *pool = NULL;
    do
    {
        pool = (threadpool_t *)malloc(sizeof(threadpool_t));
        if(pool == NULL)
        {
            printf("Failed to malloc threadpool!\n");
            break;
        }
        pool->thread_num = thread_num;
        pool->queue_num = 0;
        pool->head = 0;
        pool->tail = 0;
        
        pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*thread_num);
        if(pool->threads == NULL)
        {
            printf("Failed to malloc threads!\n");
            break;
        }

        pool->queue = (int *)malloc(sizeof(int)*queue_max_num);
        if(!pool->queue)
        {
            printf("Failed to malloc queue!\n");
            break;
        }

        if(pthread_mutex_init(&(pool->lock),NULL))
        {
            printf("Failed to init lock!\n");
            break;
        }
        if(pthread_cond_init(&(pool->queue_not_empty),NULL))
        {
            printf("Failed to init queue_not_empty!\n");
            break;
        }
        if(pthread_cond_init(&(pool->queue_not_full),NULL))
        {
            printf("Failed to init queue_not_full!\n");
            break;
        }

        for(i = 0; i < thread_num; i++)
        {
            pthread_create(&(pool->threads[i]),NULL, threadpool_thread, (void *)pool);
        }
        return(pool);
    } while (0);

    return(NULL);
}

void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    while(TRUE)
    {
        pthread_mutex_lock(&(pool->lock));

        while(pool->queue_num == 0)
        {
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
        }

        int clnt_sock = pool->queue[pool->head];
        pool->queue_num--;
        pool->head = (pool->head + 1) % MAX_QUEUE;

        if(pool->queue_num == MAX_QUEUE -1)
        {
            pthread_cond_broadcast(&(pool->queue_not_full));
        }
        
        pthread_mutex_unlock(&(pool->lock));

        handle_clnt(clnt_sock);
    }
}

int threadpool_addtask(threadpool_t *pool, int clnt_sock)
{
    pthread_mutex_lock(&(pool->lock));

    while(pool->queue_num == MAX_QUEUE)
    {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }

    pool->queue[pool->tail] = clnt_sock;
    pool->tail = (pool->tail + 1) % MAX_QUEUE;
    pool->queue_num++;

    pthread_cond_signal(&(pool->queue_not_empty));
    
    pthread_mutex_unlock(&(pool->lock));
    return(TRUE);
}

int parse_request(char* request, ssize_t req_len, char *filepath)
{
    char* req = request;
    char method[100];
    filepath[0] = '.';
    ssize_t s1 = 0;

    while(s1 < req_len && req[s1] != ' ')
    {
        method[s1] = req[s1];
        s1++;
    }
    method[s1] = '\0';

    if(strcmp(method, "GET") != 0) return(FALSE);

    ssize_t s2 = 1;
    while(s1 + s2 < req_len && req[s1 + s2] != ' ')
    {
        filepath[s2] = req[s1 + s2];
        s2++;
    }
    if(filepath[s2 - 1] == '/') return(FALSE);
    filepath[s2] = '\0';
    return(TRUE);
}

void handle_clnt(int clnt_sock)
{
    char* req_buf = (char*) malloc(MAX_RECV_LEN * sizeof(char));
    req_buf[0] = '\0';
    char* buff = (char*) malloc(MAX_RECV_LEN * sizeof(char));

    ssize_t req_len = 0;
    while(TRUE)
    {
        ssize_t len = read(clnt_sock, buff, MAX_RECV_LEN);
        if(len == 0) break;
        else
        {
            strcat(req_buf, buff);
            req_len = req_len + len;
            if(req_len >= 4 && req_buf[req_len-4] == '\r' && req_buf[req_len-3] == '\n' && req_buf[req_len-2] == '\r' && req_buf[req_len-1] == '\n') break;
        }
    }


    char *filepath = (char*) malloc(MAX_PATH_LEN * sizeof(char));
    
    int status = 0;
    ssize_t filelen = 0;  
    char *filecontent = NULL;
    if(parse_request(req_buf, req_len, filepath))
    {
        FILE *fp = fopen(filepath, "r");
        if(fp)
        {
            fseek(fp, 0, SEEK_END);
            filelen = ftell(fp);
            filecontent = (char*) malloc((filelen + 1) * sizeof(char));
            fseek(fp,0,SEEK_SET);
            fread(filecontent, 1, filelen, fp);
            filecontent[filelen] = '\0';
            fclose(fp);
            status = 200;
        } 
        else status = 404;
    }
    else status = 500;
    
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;

    switch (status)
    {
        case 200:
        {
            sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_200, filelen);
            size_t response_len = strlen(response);
            write(clnt_sock, response, response_len);
            write(clnt_sock, filecontent, filelen);
            break;
        }
        case 404:
        {
            sprintf(response, "HTTP/1.0 %s\r\n\r\n", HTTP_STATUS_404);
            size_t response_len = strlen(response);
            write(clnt_sock, response, response_len);
            break;
        }
        case 500:
        {
            sprintf(response, "HTTP/1.0 %s\r\n\r\n", HTTP_STATUS_500);
            size_t response_len = strlen(response);
            write(clnt_sock, response, response_len);
            break;
        }
        default: 
        {
            sprintf(response, "HTTP/1.0 %s\r\n\r\n", HTTP_STATUS_500);
            size_t response_len = strlen(response);
            write(clnt_sock, response, response_len);
            break;
        }
    }

    close(clnt_sock);

    free(buff);
    free(req_buf);
    free(filepath);
    free(filecontent);
    free(response);
}

int main(void)
{
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);

    threadpool_t *pool = threadpool_create(MAX_THREAD,MAX_QUEUE);

    if(bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Failed to bind!\n");
        return 255;
    } 
    if(listen(serv_sock, MAX_QUEUE) < 0) 
    {
        printf("Failed to listen!\n");

        return 255;
    }
    
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while(TRUE)
    {
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        threadpool_addtask(pool, clnt_sock);
    }

    close(serv_sock);
    return 0;
}