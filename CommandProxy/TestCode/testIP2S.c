#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
 
int
main()
{
    const char *ip_str = "192.168.1.1";
    //const char *ip_str = "255.255.255.255";
    //const char *ip_str = "0.0.0.0";
    struct in_addr in;
    memset(&in, 0, sizeof(struct in_addr));
    in.s_addr = inet_addr(ip_str);   
    printf("ip addr is:%u\n", in.s_addr);
    printf("ip addr is:%s\n", inet_ntoa(in));
    memset(&in, 0, sizeof(struct in_addr));
    inet_aton(ip_str, &in);
    printf("ip addr is:%s\n", inet_ntoa(in));
    return 0;
}