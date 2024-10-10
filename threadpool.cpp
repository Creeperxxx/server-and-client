#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>

template<typename T>
class threadpool
{
private:
    /* data */
    int m_thread_number;//线程数量
    int m_max_requests;//请求数量
    pthread_t* m_threads;//线程数组
    std::queue<T*>m_workqueue;//需求队列
    pthread_mutex_t m_mutex;//互斥量
    pthread_cond_t m_p_cond;//append请求的条件变量
    pthread_cond_t m_c_cond;//获取请求的条件变量
private:
    static void* worker(void* arg);
    void run();
public:
    void append(T* request);//将请求追加进工作队列中
    
    threadpool(int thread_number = 8,int max_requests = 1000);
    ~threadpool(); 
};
template<typename T>
threadpool<T>::threadpool(int thread_number,int max_requests):m_thread_number(thread_number),m_max_requests(max_requests)
{
    if(thread_number<=0 || max_requests<=0)
    {
        perror("threadpool()");
        exit(EXIT_FAILURE);
    }
    if(!m_threads)
    {
        perror("m_threads is zero");
    }
    for(int i = 0;i<=thread_number;i++)
    {
        if(pthread_create(m_threads+i,NULL,worker,this)!=0)
        {
            perror("threadpool::pthread_create()");
            delete[]m_threads;
            exit(EXIT_FAILURE);
        }
        if(pthread_detach(m_threads[i]))
        {
            delete[]m_threads;
            exit(EXIT_FAILURE);
        }
    }
    pthread_mutex_init(&m_mutex,NULL);
    pthread_cond_init(&m_c_cond,NULL);
    pthread_cond_init(&m_p_cond,NULL);
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[]m_threads;
    pthread_mutex_destroy(&m_mutex);
}

template<typename T>
void threadpool<T>::append(T* request)//将请求追加进工作队列中
{
    pthread_mutex_lock(&m_mutex);
    while(m_workqueue.size()>=m_max_requests)
    {
        pthread_cond_wait(&m_p_cond,&m_mutex);
    }
    m_workqueue.push(request);
    pthread_mutex_unlock(&m_mutex);
    pthread_cond_signal(&m_c_cond);
}
template<typename T>
void* threadpool<T>::worker(void * arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(true)
    {
        pthread_mutex_lock(&m_mutex);
        while(m_workqueue.empty())
        {
            pthread_cond_wait(&m_c_cond,&m_mutex);
        }
        T* request = m_workqueue.front();
        m_workqueue.pop();
        pthread_mutex_unlock(&m_mutex);
        if(!request)
        {
            continue;
        }
        //接下来就是对request进行处理

    }
}