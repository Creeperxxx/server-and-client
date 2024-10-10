#include "sql_connection_pool.h"

connection_pool::connection_pool()
{
    m_freeconn = 0;
    m_curconn = 0;
}
void connection_pool::init(string url,string user,string password,string databasename,int port,int maxconn)
{
    m_url = url;
    m_user = user;
    m_password = password;
    m_databasename = databasename;
    m_port = port;
    for(int i = 0;i<maxconn;i++)
    {
        MYSQL* con = NULL;
        MYSQL* ret = NULL;
        ret = mysql_init(con);
        if(ret == NULL)
        {
            perror("sql_connenction_pool init()");
            exit(0);
        }
        else
        {
            con = ret;
        }
        ret = mysql_real_connect(con,m_url.c_str(),m_user.c_str(),m_password.c_str(),m_databasename.c_str(),m_port,NULL,0);
        if(ret == NULL)
        {
            perror("sql_connection_pool::mysql_real_connect()");
            exit(0);
        }
        con = ret;
        connlist.push_back(con);
        ++m_freeconn;
    }
    m_maxconn = m_freeconn;
}

MYSQL* connection_pool::GetConnection()
{
    MYSQL* con = NULL;
    if(connlist.size() == 0)
    {
        return NULL;
    }
    pthread_mutex_lock(&m_mutex);
    while(connlist.size() == 0)
    {
        pthread_cond_wait(&m_c_cond,&m_mutex);
    }
    con = connlist.front();
    connlist.pop_front();
    
    --m_freeconn;
    ++m_curconn;
    pthread_mutex_unlock(&m_mutex);
    return con;
}

bool connection_pool::ReleaseConnection(MYSQL* con)
{
    if(NULL == con)
    {
        return false;
    }
    pthread_mutex_lock(&m_mutex);
    connlist.push_back(con);
    ++m_freeconn;
    --m_curconn;
    pthread_mutex_unlock(&m_mutex);
    pthread_cond_broadcast(&m_c_cond);
    return true;
}

void connection_pool::DestoryPool()
{
    pthread_mutex_lock(&m_mutex);
    if(connlist.size()>0)
    {
        list<MYSQL*>::iterator it;
        for(it = connlist.begin();it!=connlist.end();it++)
        {
            MYSQL* con = *it;
            mysql_close(con);
        }
        m_curconn = 0;
        m_freeconn = 0;
        connlist.clear();
    }
    pthread_mutex_lock(&m_mutex);
    pthread_cond_destroy(&m_c_cond);
    pthread_cond_destroy(&m_p_cond);
    pthread_mutex_destroy(&m_mutex);

}

int connection_pool::GetFreeconn()
{
    return this->m_freeconn;
}

connection_pool::~connection_pool()
{
    this->DestoryPool();
}

connection_pool* connection_pool::GetInsance()
{
    static connection_pool pool;
    return &pool;
}
connectionRAII::connectionRAII(MYSQL** con,connection_pool* coonpool)
{
    *con = coonpool->GetConnection();
    conRAII = *con;
    poolRAII = coonpool;
}

connectionRAII::~connectionRAII()
{
    poolRAII->ReleaseConnection(conRAII);
}


