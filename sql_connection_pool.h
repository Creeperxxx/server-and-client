#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <pthread.h>

using namespace std;
class connection_pool
{
public:
    string m_url;
    int m_port;
    string m_user;
    string m_password;
    string m_databasename;
private:
    connection_pool();
    ~connection_pool();
    int m_maxconn;
    int m_freeconn;
    int m_curconn;
    pthread_cond_t m_p_cond;
    pthread_cond_t m_c_cond;
    pthread_mutex_t m_mutex;
    list<MYSQL*>connlist;
public:
    MYSQL* GetConnection(); //获取数据库连接
    bool ReleaseConnection(MYSQL* conn);//释放连接
    int GetFreeconn();
    void DestoryPool();

    static connection_pool* GetInsance();
    void init(string url,string user,string password,string databasename,int port,int maxconn);
};

class connectionRAII
{
private:
    MYSQL* conRAII;
    connection_pool* poolRAII;
public:
    connectionRAII(MYSQL** con,connection_pool* connpool);
    ~connectionRAII();
};








#endif