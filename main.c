#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
    int request_number;
    struct timeval starttime;
    struct timeval endtime;
    char delay[64];
    char sip_response[248]
} measurement_t;

void evaluate_total_rtt(measurement_t *measurement, char *min, char *max, char *avg) {

}

void validate_sip_answer(measurement_t *measurement, char *buffer) {

}

void calculate_delay(measurement_t *measurement){

}

void print_progress_bar(int current, int total) {
    int bar_width = 50; 
    float progress = (float)current / total; 
    int pos = bar_width * progress; 

    printf("\r[");
    for (int i = 0; i < bar_width; ++i){
        if (i < pos) {
            printf("=");
        } else if (i == pos) {
            printf(">"); 
        } else {
            printf(" ");
        }
    }
    printf("] %d/%d Pakete", current, total);
    fflush(stdout); 
}

char *get_printable_timestamp(struct timeval tv){
    struct tm *timeinfo;
    static char timeString[100];
    char msec_buffer[8];
    timeinfo = localtime(&tv.tv_sec);

    strftime(timeString, sizeof(timeString), "%d-%m-%Y %H:%M:%S", timeinfo);
    snprintf(msec_buffer, sizeof(msec_buffer), ".%03ld", tv.tv_usec / 1000);
    strcat(timeString, msec_buffer);

    return timeString;
}

int get_options_request(char *buffer, const char *proxy, measurement_t *measurement) {
    gettimeofday(&measurement->starttime, NULL);
    snprintf(buffer, 1024,
    "OPTIONS sip:%s SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n"
    "Max-Forwards: 70\r\n"
    "From: <sip:%s>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:%s>\r\n"
    "Call-ID: jfköajbsödkivha@192.168.178.62\r\n"
    "CSeq: 10%d OPTIONS\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "User-Agent: SIP RTT-Tester by dkuehnlein\r\n"
    "X-Timestamp: %s\r\n"
    "Allow: INVITE, ACK, CANCEL, OPTIONS, BYE\r\n"
    "Content-Length: 0\r\n"
    "\r\n",
    proxy, proxy, proxy, measurement->request_number, get_printable_timestamp(measurement->starttime)
    );

    if (strlen(buffer) > 0) {
        return 0;
    } else {
        return 1;
    }
}

int main(int argc, char *argv[]) {
    int opt; 
    int sender_count = 5; 
    char *destination_ip = NULL;
    char *destination_port = "5060";
    char *export_filepath = NULL; 
    int send_summary = 0; 

    static struct option long_opts[] = {
        {"send-summary", required_argument, 0, 's'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "c:d:p:e:s:", long_opts, NULL)) != -1) {
        switch (opt) {
            case 'c': 
                sender_count = atoi(optarg); 
                break; 
            case 'd':
                destination_ip = optarg; 
                break;
            case 'p': 
                destination_port = optarg; 
                break;
            case 'e': 
                printf("Export not implemented yet. Try again later.\n"); 
                break; 
            case 's': 
                if (strcmp(optarg, "true") == 0) {
                    send_summary = 1; 
                } else if (strcmp(optarg, "True") == 0) {
                    send_summary = 1; 
                } else if (strcmp(optarg, "false") == 0) {
                    send_summary = 0; 
                } else if (strcmp(optarg, "False") == 0) {
                    send_summary = 0; 
                } else {
                    fprintf(stderr, "Unknown parameter (%s) for option '--send-summary'. Default-value (false) will be used.\n", optarg);
                    send_summary = 0; 
                }

                printf("Send-Summary: %s\n", (send_summary == 1) ? "True" : "False"); 
                break; 
            case 'h': 
                printf("Help-page work in progress\n"); 
                break;
            default: 
                printf("Man-Page work in progress\n"); 
                exit(EXIT_FAILURE);
        }
    }

    if (destination_ip == NULL) {
        fprintf(stderr, "No destination set, -d is mandatory. --help for more information.\n");
        exit(EXIT_FAILURE);
    }

    measurement_t *measurement = malloc(sender_count * sizeof(measurement_t));
    if (measurement == NULL) {
        fprintf(stderr, "Something went wrong with memory-allocation.\n");
        exit(EXIT_FAILURE);
    }

    int                 sockfd; 
    struct sockaddr_in  sockaddr;
    char                buffer[1024];
/*
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        fprintf(stderr, "Socketcreation failed.\n"); 
        exit(EXIT_FAILURE); 
    }

    memset(&sockaddr, 0x00, sizeof(sockaddr));
    sockaddr.sin_family = AF_INET; 
    sockaddr.sin_addr.s_addr = inet_addr(destination_ip); 
    sockaddr.sin_port = htons(atoi(destination_port));

    if (connect(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) !=0) {
        fprintf(stderr, "Connect to remote-destination failed.\n"); 
        close(sockfd); 
        exit(EXIT_FAILURE);
    }*/
    printf("Start measurement (%d request will be send)\n", sender_count);
    for (int i = 0; i < sender_count; i++) {
        memset(&buffer, 0x00, sizeof(buffer)); 
        print_progress_bar(i+1, sender_count);
        measurement[i].request_number = i+1;

        if (get_options_request(buffer, destination_ip, &measurement[i]) != 0) {
            fprintf(stderr, "Error while creating OPTIONS-Request occured.\n"); 
            snprintf(measurement[i].sip_response, sizeof(measurement[i].sip_response), "Error");
            shutdown(sockfd, SHUT_RDWR);
            close(sockfd); 
            exit(EXIT_FAILURE);
        }

        write(sockfd, buffer, strlen(buffer)); 
        memset(&buffer, 0x00, sizeof(buffer)); 
        read(sockfd, buffer, sizeof(buffer));
        gettimeofday(&measurement[i].endtime, NULL);
        validate_sip_answer(&measurement[i], buffer); 
        calculate_delay(&measurement[i]); 
    }

    char min[64]; 
    char max[64];
    char avg[64];
    evaluate_total_rtt(measurement, min, max, avg); 

    if (send_summary == 1) {
        //Baue SIP-OPTION mit X-Headern 
        //Begrenzung einbauen (<20 X-Header)
        //Struktur X-Header: Paket 1 // Send: HH:MM:SS.mmm // Recv: HH:MM:SS.mmm // Delay: SS.mmm
        //Letzter X-Header: Min-Delay: SS.mmm // Max-Delay: SS.mmm // Average Delay: SS.mmm
    }
    printf("\nSummary:\n"); 
    printf("Tested Proxy: %s\tPakets send: %d\n", destination_ip, sender_count);
    for (int i = 0; i < sender_count; i++) {
        printf("Paket-Nr: %d\tSend: %s\tRecv: %s\tDelay: %s\tAnswer: %s\n", 
        measurement[i].request_number,
        get_printable_timestamp(measurement[i].starttime), 
        get_printable_timestamp(measurement[i].endtime), 
        measurement[i].delay, 
        measurement[i].sip_response);
    }
    printf("===============================================================\n");
    printf("Min-Delay: %s\tMax-Delay: %s\tAverage: %s\n", min, max, avg);

    return 0; 
}