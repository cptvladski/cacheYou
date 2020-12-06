#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <stdarg.h>
#include <json.h>
#include <uuid/uuid.h>
#include "c_hashmap/hashmap.h"
#define PORT 1234


int main(int argc,char **argv){
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
    if (bind(server_fd, (struct sockaddr *)&address,sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 1) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    struct json_tokener *tokener = json_tokener_new(); 
    map_t cache = hashmap_new();
    while(1){
        char buffer[1024];
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen))<0) 
        { 
            perror("accept"); 
            exit(EXIT_FAILURE); 
        }
        read(new_socket, buffer, 1024);
        printf("got buffer: %s\n",buffer);
        if(*buffer == '{'){
            printf("inserting...\n");
            buffer[strlen(buffer) - 1] = '\0';
            struct json_object* object = json_tokener_parse_ex(tokener,buffer,strlen(buffer));
            if(object == NULL){
                printf("bad json");
                fflush(stdout);
                continue;
            }
            uuid_t binuuid;
            uuid_generate_random(binuuid);
            char * uuid = malloc(37);
            uuid_unparse_lower(binuuid,uuid);
            printf("%s\n",uuid);
            char *value = strdup(buffer);
            hashmap_put(cache,uuid,value);
            send(new_socket , uuid , strlen(uuid) , 0 );
            char * obj; 
            hashmap_get(cache,uuid,(void**)&obj);
            printf("%s\n",obj);
        }
        else{
            uuid_t binuuid;
            buffer[strlen(buffer)] = '\0';
            printf("retrieving...[len %d]%s => %d\n",strlen(buffer),buffer,uuid_parse(buffer,binuuid));

            

            if(uuid_parse(buffer,binuuid) ==-1){
                printf("bad uuid");
                fflush(stdout);
                continue;
            }
            printf("good uuid");
            char *uuid = malloc(37);
            uuid_unparse_lower(binuuid,uuid);
            char *obj;
            hashmap_get(cache,uuid,(void**)&obj);
            send(new_socket , obj , strlen(obj) , 0 );
            fflush(stdout);
        }
    }
    json_tokener_free(tokener);
    hashmap_free(cache);
	return 0;
}