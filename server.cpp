#include "head.h"

void server();
void* epollwaitdeal(void* arg);
void* clientlisten(void* arg);
void* clientdatadeal(void* arg);

int main()
{
    server();
    return 0;
}

void server()
{
    int listen_fd = socket(AF_INET,SOCK_STREAM,0);  //创建监听套接字，AF_INET:ipv4协议簇 SOCK_STREAM:套接字种类 0:默认
    if(listen_fd == -1) //判断函数调用是否成功
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    printf("成功创建监听套接字\n");
    struct sockaddr_in address;     //创建套接字地址，后面是初始化
    memset(&address,0,sizeof(address));     //地址内存处置空
    address.sin_family = AF_INET;   //ipv4协议簇
    address.sin_addr.s_addr = htonl(INADDR_ANY);    //设置所接收的IP地址 INADDR_ANY表示所有IP地址
    address.sin_port = htons(PORT); //设置端口号 服务器一般为8080
    //htonl函数和htons函数是为了将主机字节序转化为网络字节序，字节序分大小端，网络字节序规定为大端

    if(bind(listen_fd,(struct sockaddr*)&address,sizeof(address)) == -1)    //绑定套接字和地址
    {
        perror("bind");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    printf("成功绑定监听套接字地址\n");
    if(listen(listen_fd,BACKLOG) == -1) //设置监听套接字开始监听（其实在这步之前，listenfd只是个叫“监听套接字”的普通套接字
    {                                   //直到调用listen函数，该套接字就被设置为真正的监听套接字，相当于服务器大门，所有的连接请求都被
        perror("listen");               //发往这个套接字。其有两个队列：半连接队列和全连接队列，前者存储未完成三次握手的连接请求，
        close(listen_fd);               //后者存储已完成三次握手的连接请求，前者长度由内核设置，后者为listen函数参数BACKLOG
        exit(EXIT_FAILURE);             //三次请求由监听套接字与客户端完成。
    }
    int epoll_fd = epoll_create(1); //epoll可以监听多个文件描述符上的特定事件是否发生 调用epoll_create创建一个epoll实例
    if(epoll_fd == -1)              //我们用epoll实例来监听 监听套接字上的读事件，就是说，如果监听套接字上由连接请求可读，
    {                               //使用epoll相关函数可以监听到
        perror("epollcreate");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;  //实例化一个epoll事件对象，epoll事件包含两个因素：文件描述符和其上发生的某个事件 用来配置epoll实例
                            //要监听哪个文件描述符上的什么事件
    ev.events = EPOLLIN;    //设置读事件，也可以设置模式为ls水平触发还是et边缘触发后面会讲
    ev.data.fd = listen_fd; //设置所监听的文件描述符，这里设置为监听套接字
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&ev) == -1)   //设置好ev变量后，用这个变量配置epoll实例，告诉它监听哪个
    {                                                           //文件描述符上什么事件。第二个参数为模式，分为添加，删除，修改
        perror("epollctl");
        close(epoll_fd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    printf("成功创建epoll实例并监听listen套接字\n");

    pthread_t epollwaitthread;
    int epoll_wait_threads[2]={epoll_fd,listen_fd};
    if(pthread_create(&epollwaitthread,NULL,epollwaitdeal,epoll_wait_threads)!=0)
    {
        perror("pthread_create");
        close(epoll_fd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        sleep(5);
        printf("主线程正在做自己的事\n");
    }
    pthread_join(epollwaitthread,NULL);

    close(listen_fd);
    close(epoll_fd);
}

void* epollwaitdeal(void * arg)
{
    printf("epollwaitdead线程创建\n");
    int* data = (int*)arg;
    int epoll_fd = data[0];
    int listen_fd = data[1];
    struct epoll_event events[MAX_EVENTS];
    struct sockaddr_in client_addr;
    int clientaddrlen = sizeof(client_addr);
    while(1)
    {
        int nfds = epoll_wait(epoll_fd,events,MAX_EVENTS,-1);
        for(int i = 0;i<nfds;i++)
        {
            if(events[i].data.fd == listen_fd)
            {
                pthread_t addclientepoll;
                if(pthread_create(&addclientepoll,NULL,clientlisten,data)!=0)
                {
                    perror("pthread_create: clientlisten");
                    continue;;
                }
            }
            else
            {
                pthread_t dealclientdata;
                if(pthread_create(&dealclientdata,NULL,clientdatadeal,&events[i].data.fd) !=0)
                {
                    perror("pthread_create:clientdatadeal");
                    continue;
                }
            }
        }
    }
    pthread_exit(NULL);

}

void* clientlisten(void* arg)
{
    printf("clientlisten线程创建成功\n");
    int* data = (int*)arg;
    int epoll_fd = data[0];
    int listen_fd = data[1];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int newsocket = accept(listen_fd,(struct sockaddr*)&client_addr,&client_addr_len);
    if(newsocket == -1)
    {
        perror("client newsocket");
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    struct epoll_event ev;
    ev.events = EPOLLIN|EPOLLET;
    ev.data.fd = newsocket;
    if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,newsocket,&ev) == -1)
    {
        perror("client newsocket epoll_ctl add");
        close(epoll_fd);
        close(listen_fd);
        exit(EXIT_FAILURE);
    }
    printf("已与%s:%d建立连接\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    pthread_exit(NULL);

}

void* clientdatadeal(void* arg)
{
    printf("clientdatadeal线程创建成功\n");
    int client_socket = *(int*)arg;
    char buffer[CLIENT_BUFFER_SIZE];

    struct sockaddr_in client_addr;
    socklen_t addrlen = sizeof(client_addr);
    if(getpeername(client_socket,(struct sockaddr*)&client_addr,&addrlen) == -1)
    {
        perror("clientdatadeal:nums=0:getpeername");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    uint16_t host_port = ntohs(client_addr.sin_port);

    int nums = read(client_socket,buffer,CLIENT_BUFFER_SIZE-1);
    if(nums == -1)
    {
        perror("clientdatadeal read");
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    else if (nums == 0)
    {
        /* code */
        
        printf("客户端%s:%d关闭连接\n",ip_str,host_port);
    }
    else    
    {
        buffer[nums] = '\0';
        printf("收到来自客户端%s:%d的数据：%s",ip_str,host_port,buffer);
        if(send(client_socket,receive_buffer,strlen(receive_buffer),0) == -1)
        {
            perror("clientdatadeal:num>0:send");
            close(client_socket);
            exit(EXIT_FAILURE);
        }
    }
    pthread_exit(NULL);
  
}
