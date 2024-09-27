#include "head.h"

void client()
{
    int sockfd = socket(AF_INET,SOCK_STREAM,0); //创建一个套接字用于与服务器相互通信
    if(sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    printf("创建套接字\n");
    struct sockaddr_in address; //配置套接字地址
    address.sin_family = AF_INET;   //ipv4协议簇
    address.sin_port = htons(PORT); //设为服务器的监听套接字端口 htons函数和网络字节序相关
    if(inet_pton(AF_INET,SERVER_IP,&address.sin_addr.s_addr) <= 0)  //服务器ip转化为网络字节序配置address
    {
        perror("set serverip");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("套接字成功绑定地址\n");
    if(connect(sockfd,(struct sockaddr*)&address,sizeof(address)) == -1)    //将该套接字与服务器相连接
    {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("成功连接到%s:%d\n",SERVER_IP,PORT);
    
    //发送数据到服务器
    const char * data_buffer = "hello ,server\n";
    if(send(sockfd,data_buffer,strlen(data_buffer),0) == -1)    //发送数据
    {
        perror("data send");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    printf("发送了数据：%s",data_buffer);

    //收到服务器的回复数据
    char receiveddata_buffer[CLIENT_BUFFER_SIZE];
    ssize_t data_received = recv(sockfd,receiveddata_buffer,CLIENT_BUFFER_SIZE - 1,0);//等待接收数据
    if(data_received == -1)
    {
        perror("client recv");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    else if(data_received == 0)
    {
        printf("connection closed");
    }
    receiveddata_buffer[data_received] = '\0';
    printf("成功收到数据：%s\n",receiveddata_buffer);
    close(sockfd);
    printf("客户端关闭\n");
}

int main()
{
    client();
}