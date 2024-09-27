#include "head.h"

void server();

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
    struct epoll_event events[MAX_EVENTS];  //epoll事件数组，epoll_wait函数会用到
    struct sockaddr_in client_addr; //客户端套接字地址
    char buffer[1024];  //缓冲区，存放接收到的信息
    socklen_t addrlen = sizeof(client_addr);    //客户端套接字地址的大小

    while(1)    //循环调用epoll_wait
    {
        int nfds = epoll_wait(epoll_fd,events,MAX_EVENTS, -1 );
        if(nfds == -1)              //epoll_wait函数，作用为每调用一次，会检查 epoll实例监听的事件是否发生（是否为就绪事件），例如
        {                           //监听套接字上的连接请求在自己的缓冲区内，如果epoll实例监听该套接字的读事件，那么只要该缓冲区
            perror("epollwait");    //由数据可读，读事件就属于就绪事件，会被epoll_wait检测到。该函数会将所有的就绪事件复制到events
            close(listen_fd);       //数组内存放，所以events数组的类型为epoll事件，MAX_EVENTS为数组的大小。若检测时发现没有就绪事件，
            close(epoll_fd);        //则会阻塞，就是一直等待在那里，直到有就绪事件发生。最后一个参数为定时器，表明阻塞的最大时间。-1为
            exit(EXIT_FAILURE);     //无限期阻塞。该函数会返回就绪事件的个数（不是套接字的个数）。至于循环调用epoll_wait，是不让其
        }                           //只有一次监听到就绪事件的机会，让其能够无限次监听到就绪事件的次数，
        for(int i =0 ; i < nfds ; ++i)  //挨个处理events数组中存储的就绪事件
        {
            if(events[i].data.fd == listen_fd)  //取出一个就绪事件，判断这个事件的套接字是否为监听套接字。
            {                                   
                int conn_fd = accept(listen_fd,(struct sockaddr*)&client_addr,&addrlen);//该函数是取出一个监听套接字全连接队列中
                if(conn_fd == -1)           //连接请求，并将发送连接请求的客户端的套接字地址存入client_addr，后者为地址长度                                                    
                {                           //函数会返回一个新套接字，与该客户端相通信
                    perror("accept");
                    continue;
                }
                //接下来将这个处理这个新套接字，其实就是用epoll实例去监听它，判断客户端是否有向该套接字发送信息
                ev.events = EPOLLIN;        //监听新套接字的读事件
                ev.data.fd = conn_fd;       //设置为新套接字
                if(epoll_ctl(epoll_fd,EPOLL_CTL_ADD,conn_fd,&ev) == -1)     //用设置好的ev配置epoll实例，向epoll实例添加监听对象
                {
                    perror("epollctl addclientfd");
                    close(listen_fd);
                    close(epoll_fd);
                    exit(EXIT_FAILURE);
                }
                printf("成功与%s:%d连接\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            }
            else    //我们只让epoll实例监听两种套接字：监听套接字和与客户端连接的套接字，既然不是监听套接字，那肯定就是与客户端
            {       //连接的套接字了
                //客户端上有可读事件发生
                printf("检测到可读事件\n");
                int bytes_read = recv(events[i].data.fd,buffer,sizeof(buffer) - 1,0);//从套接字的缓冲区将数据读入buffer中
                if(bytes_read == -1)                                                 //返回读取到的字节个数
                {
                    perror("read data form client");
                    close(events[i].data.fd);
                    continue;
                }
                else if(bytes_read == 0)        //如果读事件为就绪事件但读到0个字节，说明客户端套接字关闭连接
                {
                    //客户端关闭连接
                    printf("客户端关闭连接\n");
                    close(events[i].data.fd);
                    continue;
                }
                buffer[bytes_read]='\0';    //在缓冲区的最后添加一个\0，方便打印字符串
                printf("服务端接收到数据：%s",buffer);
                if(send(events[i].data.fd,receive_buffer,strlen(receive_buffer),0)==-1) //发送一个收到信息给客户端 
                {
                    perror("send received data");
                    close(events[i].data.fd);
                    continue;
                }
                printf("向客户端发送确认信息\n");
            }
        }
    }
    close(listen_fd);
    close(epoll_fd);
}

