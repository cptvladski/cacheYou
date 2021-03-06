#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>
#include <pthread.h>
#include <json.h>
#include "worker.h"
#define PORT 1234

map_t cache;
struct json_tokener *tokener;
lifetime_stack_t *trash;
void sigint_handler(int signum){
    printf("[main] freeing all resources and exiting...\n");
    lifetime_destroy(trash);
    exit(0);
}

int main(int argc,char **argv){
    signal(SIGINT,sigint_handler);
    trash = create_stack(1000);
    char hostname[100];
    gethostname(hostname,100);
    printf("[main]%s cacheyou container starting...\n",hostname);
    cache = hashmap_new();
    tokener = json_tokener_new(); 
    res_add(trash,cache,hashmap_free);
    res_add(trash,tokener,(void (*)(void *))json_tokener_free);
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int new_socket;
    int addrlen = sizeof(struct sockaddr_in);
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    // Forcefully attaching socket to the port 1234
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
    //bind socket to port
    if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    //listen for connection on port
    if (listen(server_fd, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    
    printf("[main] starting accept loop\n");
    fflush(stdout);
    while(1){
        // accept new connection
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }
        pthread_t tid;
        args_t arg;
        arg.fd = new_socket;
        pthread_create(&tid,NULL,workerThread,(void*) &arg);
        res_add(trash,&tid,join_wrapper);
    }
    return 0;
}
