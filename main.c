#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

const long TRANSLATE_FACTOR = 1000000000L;

typedef struct {
    int request_number;
    struct timespec sendtime;
    struct timespec recvtime;
    long total_delay;
    char sip_response[248];
} measurement_t;

void evaluate_total_rtt(measurement_t *measurement, char *min, char *max, char *avg, int sender_count) {
    long temp_min = measurement[0].total_delay;
    long temp_max = 0;
    long total = 0; 
    for (int i = 0; i < sender_count; i++) {
        if (measurement[i].total_delay < temp_min) {
            temp_min = measurement[i].total_delay;
        }
        if (measurement[i].total_delay > temp_max) {
            temp_max = measurement[i].total_delay;
        }
        total += measurement[i].total_delay;
    }
    long temp_avg = total / sender_count;

    snprintf(min, 64, "%ld.%03ld", temp_min / TRANSLATE_FACTOR, temp_min % TRANSLATE_FACTOR);
    snprintf(max, 64, "%ld.%03ld", temp_max / TRANSLATE_FACTOR, temp_max % TRANSLATE_FACTOR);
    snprintf(avg, 64, "%ld.%03ld", temp_avg / TRANSLATE_FACTOR, temp_avg % TRANSLATE_FACTOR);
    
}

void validate_sip_answer(measurement_t *measurement, char *buffer) {
    char *status_line_start = strstr(buffer, "SIP/2.0"); 
    char *status_line_end = strstr(buffer, "\r\n"); 
    if (status_line_start && status_line_end) {
        snprintf(measurement->sip_response, sizeof(measurement->sip_response), 
        "%.*s", (int)(status_line_end - (status_line_start + 8)), status_line_start + 8);
    } else {
        fprintf(stderr, "No valid status-line detected in response.\n"); 
        snprintf(measurement->sip_response, sizeof(measurement->sip_response), "invalid response"); 
    }
}

void calculate_delay(measurement_t *measurement){
    long seconds = measurement->recvtime.tv_sec - measurement->sendtime.tv_sec;
    long nanosec = measurement->recvtime.tv_nsec - measurement->sendtime.tv_nsec;

    if (nanosec < 0) {
        seconds -= 1; 
        nanosec += 1000000000;
    }
    measurement->total_delay = seconds * TRANSLATE_FACTOR + nanosec;

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

void get_printable_timestamp(struct timespec ts, char *buffer, const int format_long, size_t buffersize) {
    struct tm *timeinfo;
    char msec_buffer[64];
    timeinfo = localtime(&ts.tv_sec);

    if (format_long == 1) {
        strftime(buffer, buffersize, "%d-%m-%Y %H:%M:%S", timeinfo);
        snprintf(msec_buffer, sizeof(msec_buffer), ".%06ld", ts.tv_nsec/1000);
        strncat(buffer, msec_buffer, buffersize-strlen(buffer)-1);
    } else if (format_long == 0) {
        strftime(buffer, buffersize, "%H:%M:%S", timeinfo);
        snprintf(msec_buffer, sizeof(msec_buffer), ".%06ld", ts.tv_nsec/1000);
        strncat(buffer, msec_buffer, sizeof(buffer)-strlen(buffer)-1);
    } else {
        fprintf(stderr, "Unrecognized format-option.\n");
        strncpy(buffer, "Unavail", buffersize); 
    }
}

int get_options_request(char *buffer, const char *proxy, measurement_t *measurement) {
    clock_gettime(CLOCK_REALTIME, &measurement->sendtime);
    char read_time [100];
    get_printable_timestamp(measurement->sendtime, read_time, 1, sizeof(read_time));
    snprintf(buffer, 1024,
    "OPTIONS sip:%s SIP/2.0\r\n"
    "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n"
    "Max-Forwards: 70\r\n"
    "From: <sip:%s>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
    "To: <sip:%s>\r\n"
    "Call-ID: jfklajbspdkivha@192.168.178.62\r\n"
    "CSeq: 10%d OPTIONS\r\n"
    "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
    "User-Agent: SIP RTT-Tester by dkuehnlein\r\n"
    "X-Timestamp: %s\r\n"
    "Content-Length: 0\r\n\r\n",
    proxy, proxy, proxy, measurement->request_number, read_time
    );

    if (strlen(buffer) > 0) {
        return 0;
    } else {
        return 1;
    }
}

void print_help() {
    printf("Usage: sip_rtt_analyzer [OPTION][VALUE]...\n");
    printf("Measuring the time between sending an SIP-OPTION requests and receiving the corresponding response.\n\n");
    printf("Mandatory-Arguments are labled with *\n");
    printf("  -c,                     Number of requests to be sent (default: 5)\n");
    printf("  -s,                     Set the sleep-timer between every paket (default: 500ms)\n");
    printf("                          Becareful: unit is mikroseconds\n");
    printf(" *-d,                     Destination ip-address of sip-proxy (format: xxx.xxx.xxx.xxx)\n");
    printf("  -p,                     Destination port for the requests (default: 5060)\n");
    printf("  --send-summary[=true]   If set to 'true' the summary will be sent as a separate OPTION-Request\n"); 
    printf("                          to the proxy. Every requests has it's own X-Header with varios information.\n"); 
    printf("                          Only the first ten pakets will be send. For more information see the man-page.\n");
    printf("                          Default-value: false\n");
    printf("  --version               Show version\n"); 
    printf("  --help                  Show help\n\n"); 
    printf("Attention: there is no explicit implemented security-mechanism to protect the sip-proxy.\n");
    printf("The user have the full responsibility to not overload the sip-proxy.\n");
}

int main(int argc, char *argv[]) {
    int opt; 
    int sender_count = 5; 
    char *destination_ip = NULL;
    char *destination_port = "5060";
    int send_summary = 0; 
    unsigned int sleep_timer = 500000;

    static struct option long_opts[] = {
        {"send-summary", required_argument, 0, 'a'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
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
            case 's': 
                sleep_timer = atoi(optarg); 
                break;
            case 'a': 
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
                break; 
            case 'h': 
                print_help();
                exit(EXIT_SUCCESS);  
            case 'v': 
                printf("Version: 1.0.1\n"); 
                exit(EXIT_SUCCESS);
            default: 
                fprintf(stderr, "Error: Unknown argument. Use --help for more information.\n");
                exit(EXIT_FAILURE);
        }
    }

    if (destination_ip == NULL) {
        fprintf(stderr, "No destination set, -d is mandatory. --help for more information.\n");
        exit(EXIT_FAILURE);
    }

    if (send_summary && sender_count > 10) {
        printf("Warning: To avoid too large OPTION-Requests the summary-message is limited to a maximum of 10 summary-rows - \n"
        "you can still send more than 10 requests.\n");
        if (sleep_timer < 500000) {
            char answer;
            printf("Attention: You're trying to send a high amount of requests (>10) in a very short time.\n"); 
            printf("Please make sure you know what you do and the tested proxy is able to handle this.\n"); 
            printf("If something get wrong, you're alone responsible.\n"); 
            printf("Continue? (y/n) "); 
            scanf("%c", &answer);
            if (answer != 'y') {
                printf("Aborted\n"); 
                exit(EXIT_FAILURE); 
            }
        }
    }

    measurement_t *measurement = malloc(sender_count * sizeof(measurement_t));
    if (measurement == NULL) {
        fprintf(stderr, "Something went wrong with memory-allocation.\n");
        exit(EXIT_FAILURE);
    }

    int                 sockfd; 
    struct sockaddr_in  sockaddr;
    char                buffer[1024];

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
    }

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
        clock_gettime(CLOCK_REALTIME, &measurement[i].recvtime);
        validate_sip_answer(&measurement[i], buffer); 
        calculate_delay(&measurement[i]); 
        usleep(sleep_timer);
    }

    char min[64]; 
    char max[64];
    char avg[64];
    int sum_row_count; 
    evaluate_total_rtt(measurement, min, max, avg, sender_count); 

    if (send_summary == 1) {
        if (sender_count > 10) {
            sum_row_count = 10; 
        } else {
            sum_row_count = sender_count;
        }

        char summary_message[2048];
        char x_header[1024];

        for (int i = 0; i < sum_row_count; i++) {
            char sum_buffer[640];
            char send_time[100];
            char recv_time[100]; 

            get_printable_timestamp(measurement[i].sendtime, send_time, 0, sizeof(send_time));
            get_printable_timestamp(measurement[i].recvtime, recv_time, 0, sizeof(recv_time)); 

            snprintf(sum_buffer, sizeof(sum_buffer), 
            "X-Sum: Pkt %d / Send: %s / Recv: %s / Delay: %ld.%03ld / Resp: %s\r\n", 
            measurement[i].request_number, 
            send_time, 
            recv_time,
            measurement[i].total_delay / TRANSLATE_FACTOR,
            measurement[i].total_delay % TRANSLATE_FACTOR, 
            measurement[i].sip_response);

            strncat(x_header, sum_buffer, sizeof(x_header)-strlen(x_header));
        }
        snprintf(summary_message, sizeof(summary_message), 
        "OPTIONS sip:%s SIP/2.0\r\n"
        "Via: SIP/2.0/TCP 192.168.178.62:5060;branch=z9hG4bK776asdhds\r\n"
        "Max-Forwards: 70\r\n"
        "From: <sip:%s>;tag=djaiefkla348afikju3u9dkhjk3\r\n"
        "To: <sip:%s>\r\n"
        "Call-ID: jfklajbspdkivha@192.168.178.62\r\n"
        "CSeq: 200 OPTIONS\r\n"
        "Contact: <sip:+4919952000234234@192.168.178.62:5060;transport=tcp>\r\n"
        "User-Agent: SIP RTT-Tester by dkuehnlein\r\n"
        "X-Total: Min-Delay: %s / Max-Delay: %s / Avg-Delay: %s\r\n"
        "%s"
        "Content-Length: 0\r\n\r\n",
        destination_ip, destination_ip, destination_ip, min, max, avg, x_header);

        write(sockfd, summary_message, strlen(summary_message)); 
    }

    printf("\nFinished sending\n"); 
    shutdown(sockfd, SHUT_RDWR);
    close(sockfd); 

    printf("\nSummary:\n"); 
    printf("Tested Proxy: %s\tPakets send: %d\n", destination_ip, sender_count);
    for (int i = 0; i < sender_count; i++) {
        char send_time[100];
        char recv_time[100]; 
        get_printable_timestamp(measurement[i].sendtime, send_time, 0, sizeof(send_time));
        get_printable_timestamp(measurement[i].recvtime, recv_time, 0, sizeof(recv_time)); 

        printf("Paket-Nr: %d\tSend: %s\tRecv: %s\tDelay: %ld.%03ld\tAnswer: %s\n", 
        measurement[i].request_number,
        send_time, 
        recv_time, 
        measurement[i].total_delay / TRANSLATE_FACTOR,
        measurement[i].total_delay % TRANSLATE_FACTOR, 
        measurement[i].sip_response);
    }
    printf("===============================================================\n");
    printf("Min-Delay: %s\tMax-Delay: %s\tAverage: %s\n", min, max, avg);

    return 0; 
}