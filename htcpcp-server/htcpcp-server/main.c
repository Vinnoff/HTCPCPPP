#include<stdio.h>
#include<sys/types.h>//socket
#include<sys/socket.h>//socket
#include<string.h>//memset
#include<stdlib.h>//sizeof
#include<netinet/in.h>//INADDR_ANY
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#define PORT 8000
#define NUM_CLIENT 5
#define MAXSZ 1000
#define COFFEE_TIME 50

struct Request request;

struct Header {
    char* key;
    char* value;
};

struct Request{
    char* method;
    char* path;
    struct Header* headers;
    int headerSize;
};

struct Coffee{
    int secondsTillEnd;
    int containsHeaders;
    char* coffee_type;
    char* milk_type;
    char* syrup_type;
    char* alcohol_type;
    char* sugar_type;
};

char* coffee_type[3]  = { "/darjeeling", "/earl-grey", "/peppermint" };
char* milk_type[6]    = { "Cream", "Half-and-half", "Whole-milk" , "Part-Skim", "Skim", "Non-Dairy" };
char* syrup_type[4]   = { "Vanilla", "Almond", "Raspberry" , "Chocolate" };
char* alcohol_type[4] = { "Whisky", "Rum", "Kahlua", "Aquavit" };
char* sugar_type[3]   = { "Sugar", "Xylitol", "Stevia" };

int is_making_coffee;
struct Coffee coffee;
pthread_t* coffee_thread;
int waiting_client_socket;
struct Coffee available_coffees[10];
int number_of_coffees = 0;

int add_header(struct Request request, struct Header header) {
    request.headers[request.headerSize] = header;
    return request.headerSize+1;
}

void printRequest(struct Request request){
    char* to_print = malloc(1000);
    strcpy(to_print,"request :");
    strcat(to_print, "\n\tmethod : ");
    if(request.method != NULL)
        strcat(to_print, request.method);
    else
        strcat(to_print, "NULL");
    strcat(to_print, "\n\tpath : ");
    if(request.path != NULL)
        strcat(to_print, request.path);
    else
        strcat(to_print, "NULL");
    strcat(to_print, "\n");
    for(int i = 0; i < request.headerSize; i++){
        strcat(to_print, "\theader " );
        char str[12];
        sprintf(str, "%d", i+1);
        strcat(to_print, str);
        strcat(to_print, ": {" );
        strcat(to_print, request.headers[i].key );
        strcat(to_print, " : " );
        strcat(to_print, request.headers[i].value );
        strcat(to_print, "}\n" );
    }
    printf("%s", to_print);
    free(to_print);
}

static int compareString(char *msg, char *waited) {
    if(strncmp(msg, waited, sizeof(msg)) == 0){
        return 1;
    } else {
        return 0;
    }
}

void stopCoffee(){
    system("python ~/test.py");
    pthread_cancel(coffee_thread);
    is_making_coffee = 0;
}

void hardStopCoffee(){
    stopCoffee();
    send(waiting_client_socket,"Coffee stopped",MAXSZ,0);
}

void *startCoffee(int newsockfd){
    waiting_client_socket = newsockfd;
    signal(SIGTSTP, hardStopCoffee);
    send(newsockfd,"Coffee is making",MAXSZ,0);
    system("python ~/test.py");
    for(int i = COFFEE_TIME; i > 0; i--){
        printf("%d seconds 'till coffee ready\n", i);
        coffee.secondsTillEnd = i;
        sleep(1);
    }
    available_coffees[number_of_coffees] = coffee;
    printf("Coffee n° %d ready\n", ++number_of_coffees);
    stopCoffee();
    return NULL;
}

static void set_coffee_parameters() {
    coffee.coffee_type = request.path;
    if(request.headerSize > 0){
        for (int i = 0; i < request.headerSize; i++) {
            if (strcmp(request.headers[i].key, "--milk") == 0){
                coffee.milk_type = request.headers[i].value;
            }
            else if (strcmp(request.headers[i].key, "--syrup") == 0){
                coffee.syrup_type = request.headers[i].value;
            }
            else if (strcmp(request.headers[i].key, "--alcohol") == 0){
                coffee.alcohol_type = request.headers[i].value;
            }
            else if (strcmp(request.headers[i].key, "--sugar") == 0){
                coffee.sugar_type = request.headers[i].value;
            }
        }
        coffee.containsHeaders = 1;
    } else
        coffee.containsHeaders = 0;
}

void requestPost(int newsockfd){
    for (int i = 0; i<sizeof(coffee_type)/sizeof(char*); i++) {
        if(compareString(request.path, coffee_type[i]) == 1){
            if(is_making_coffee == 0){
                if(number_of_coffees == 10){
                    send(newsockfd,"too much coffees, take one with \"GET \\number\" before",MAXSZ,0);
                }
                is_making_coffee = 1;
                set_coffee_parameters();
                if(pthread_create(&coffee_thread, NULL, startCoffee, newsockfd) != 0){
                    perror("Error creating thread");
                }
            } else{
                send(newsockfd,"Already making coffee",MAXSZ,0);
            }
            return;
        }
    }
    send(newsockfd,"Invalid Coffee",MAXSZ,0);
}

char* print_coffee(struct Coffee coffee){
    char* to_print = malloc(100);
    strcat(to_print, "type : ");
    strcat(to_print, coffee.coffee_type);
    if (coffee.milk_type != NULL){
        strcat(to_print, "\nmilk : ");
        strcat(to_print, coffee.milk_type);
    }
    if (coffee.sugar_type) {
        strcat(to_print, "\nsugar : ");
        strcat(to_print, coffee.sugar_type);
    }
    if (coffee.alcohol_type) {
        strcat(to_print, "\nalcohol : ");
        strcat(to_print, coffee.alcohol_type);
    }
    if (coffee.syrup_type) {
        strcat(to_print, "\nsyrup : ");
        strcat(to_print, coffee.syrup_type);
    }
    strcat(to_print, "\n");
    
    return to_print;
}

static struct Coffee delete_coffee_from_available(int x) {
    struct Coffee to_return = available_coffees[x];
    for(int i = x; i < number_of_coffees - 1; i++){
        available_coffees[i] = available_coffees[i+1];
    }
    available_coffees[number_of_coffees - 1].coffee_type = NULL;
    available_coffees[number_of_coffees - 1].milk_type = NULL;
    available_coffees[number_of_coffees - 1].syrup_type = NULL;
    available_coffees[number_of_coffees - 1].sugar_type = NULL;
    available_coffees[number_of_coffees - 1].alcohol_type = NULL;
    available_coffees[number_of_coffees - 1].containsHeaders = 0;
    printf("Coffee n°%d given, %d available\n", x+1, --number_of_coffees);
    return to_return;
}

void requestGet(int newsockfd){
    if(compareString(request.path, "/") == 1){
        if(number_of_coffees == 0) {
            send(newsockfd,"204 no.coffee\0",MAXSZ,0);
            return;
        }
        char* str = malloc(100 * number_of_coffees);
        strcpy(str, "");
        for (int i = 0 ; i < number_of_coffees; i++) {
            strcat(str, "coffee ");
            char toString[12];
            sprintf(toString, "%d", i+1);
            strcat(str, toString);
            strcat(str, " :\n");
            strcat(str, print_coffee(available_coffees[i]));
        }
        send(newsockfd,str,MAXSZ,0);
    } else {
        char* path = request.path;
        strsep(&path, "/");
        if(atoi(path) > 0 && atoi(path) <= number_of_coffees){
            send(newsockfd, print_coffee(delete_coffee_from_available(atoi(path) - 1)), MAXSZ, 0);
        } else {
            send(newsockfd,"406 not.acceptable\0",MAXSZ,0);
        }
    }
}

void requestPropfind(int newsockfd){
    send(newsockfd,"Making coffee dumbass\0",MAXSZ,0);
}

void requestWhen(int newsockfd){
    if (is_making_coffee) {
        hardStopCoffee();
        send(newsockfd,"Coffee stopped\0",MAXSZ,0);
    } else
        send(newsockfd,"No coffee started\0",MAXSZ,0);
}

void requestNA(int newsockfd){
    send(newsockfd,"406 not.acceptable\0",MAXSZ,0);
}

static void extractCode(char *msg, int newsockfd) {
    request.method = strsep(&msg, "=");
    request.path = strsep(&msg, "=");
    if(request.path == NULL){
        requestNA(newsockfd);
        return;
    }
    request.headerSize = 0;
    request.headers = malloc(sizeof(struct Header) * 10);
    
    while (1){
        struct Header header;
        header.key = strsep(&msg, "=");
        if(header.key == NULL)
            break;
        header.value = strsep(&msg, "=");
        if(header.value == NULL){
            break;
        }
        request.headerSize = add_header(request, header);
    }
    
    printRequest(request);
    
    if(compareString(request.method, "BREW") || compareString(request.method, "POST")){
        requestPost(newsockfd);
    } else if(compareString(request.method, "GET")){
        requestGet(newsockfd);
    } else if(compareString(request.method, "PROPFIND")){
        requestPropfind(newsockfd);
    } else if(compareString(request.method, "WHEN")){
        requestWhen(newsockfd);
    } else {
        requestNA(newsockfd);
    }
}


void *connection_handler(void *newsockfd)
{
    int sock = *(int *)newsockfd;
    printf("Handling messages from sock %d\n", sock);
    char* msg = malloc(MAXSZ);
    int read_size;
    
    while((read_size = read(sock, msg, MAXSZ) > 0)){
        if(read_size == 0){
            puts("Client disconnected");
            fflush(stdout);
            break;
        } else if (read_size == -1){
            perror("recv failed");
            break;
        }
        extractCode(msg, sock);
    }
    free(newsockfd);
    
    return NULL;
}

int main()
{
    int listenfd, connefd, *new_sock;
    socklen_t clientlen;
    struct sockaddr_in clientaddr, servadrr;
    
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if(listenfd == -1){
        perror("Could not create socket");
    }
    
    printf("Socket Created\n");
    
    bzero(&servadrr, sizeof(servadrr));
    servadrr.sin_family=AF_INET;
    servadrr.sin_addr.s_addr=htonl(INADDR_ANY);
    servadrr.sin_port=htons(PORT);
    
    if(bind(listenfd,(struct sockaddr *)&servadrr, sizeof(servadrr)) < 0){
        perror("bind failed");
        return 1;
    }
    printf("bind success\n");
    
    listen(listenfd, NUM_CLIENT);
    
    printf("Waiting for connections\n");
    clientlen = sizeof(clientaddr);
    
    while ((connefd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen))) {
        printf("connection accepted from client: %s\n",inet_ntoa(clientaddr.sin_addr));
        pthread_t server_thread;
        new_sock = malloc(1);
        *new_sock = connefd;
        pthread_create(&server_thread, NULL, connection_handler, (void*) new_sock);
    }
    
    if(connefd < 0){
        perror("Accept failed");
        return 1;
    }
    return 0;
}
