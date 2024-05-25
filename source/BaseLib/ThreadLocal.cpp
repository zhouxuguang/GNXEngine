//
//  ThreadLocal.cpp
//  BaseLib
//
//  Created by Zhou,Xuguang on 2018/1/21.
//  Copyright © 2018年 zhouxuguang. All rights reserved.
//

#include "ThreadLocal.h"

NS_BASELIB_BEGIN

namespace internal
{
    
    typedef void (*tls_dtor_t)(void*);
    
    //基本的跨平台线程局部存储包装类
    template <typename T>
    class basic_tls
    {
#if defined(__linux__) || defined(__APPLE__)
        typedef pthread_key_t tls_key_t;
    public:
        int  create( tls_dtor_t dtor = NULL )
        {
            return pthread_key_create(&my_key, dtor);
        }
        int  destroy()      { return pthread_key_delete(my_key); }
        void set( T value ) { pthread_setspecific(my_key, (void*)value); }
        T    get()          { return (T)pthread_getspecific(my_key); }
        
#elif defined(WIN32) /* USE_WINTHREAD */
        typedef DWORD tls_key_t;
    public:
#if !__TBB_WIN8UI_SUPPORT
        int create()
        {
            tls_key_t tmp = TlsAlloc();
            if( tmp==TLS_OUT_OF_INDEXES )
                return TLS_OUT_OF_INDEXES;
            my_key = tmp;
            return 0;
        }
        int  destroy()      { TlsFree(my_key); my_key=0; return 0; }
        void set( T value ) { TlsSetValue(my_key, (LPVOID)value); }
        T    get()          { return (T)TlsGetValue(my_key); }
#else /*!__TBB_WIN8UI_SUPPORT*/
        int create()
        {
            tls_key_t tmp = FlsAlloc(NULL);
            if( tmp== (DWORD)0xFFFFFFFF )
                return (DWORD)0xFFFFFFFF;
            my_key = tmp;
            return 0;
        }
        int  destroy()      { FlsFree(my_key); my_key=0; return 0; }
        void set( T value ) { FlsSetValue(my_key, (LPVOID)value); }
        T    get()          { return (T)FlsGetValue(my_key); }
#endif /* !__TBB_WIN8UI_SUPPORT */
#endif /* USE_WINTHREAD */
    private:
        tls_key_t my_key;
    };
    
    //! More advanced TLS support template class.
    /** It supports RAII and to some extent mimic __declspec(thread) variables. */
    template <typename T>
    class tls : public basic_tls<T> 
    {
        typedef basic_tls<T> base;
    public:
        tls()  { base::create();  }
        ~tls() { base::destroy(); }
        T operator=(T value) { base::set(value); return value; }
        operator T() { return base::get(); }
    };
    
    //TLS包装类
    class ThreadLocalStorage
    {
    public:
        ThreadLocalStorage(tls_dtor_t pDestroyFun)
        {
#if defined(__linux__) || defined(__APPLE__)
            mTls.create(pDestroyFun);
#else
            mTls.create();
#endif
        }
        
        ~ThreadLocalStorage()
        {
            mTls.destroy();
        }
        
        void set( void* value )
        {
            mTls.set(value);
        }
        
        void* get()
        {
            return mTls.get();
        }
        
    private:
        basic_tls<void*> mTls;
    };
    
    template <typename T>
    class tls<T*> : basic_tls<T*>
    {
        typedef basic_tls<T*> base;
        static void internal_dtor(void* ptr)
        {
            if (ptr) delete (T*)ptr;
        }
        
        T* internal_get()
        {
            T* result = base::get();
            if (!result)
            {
                result = new T;
                base::set(result);
            }
            return result;
        }
    public:
        tls()
        {
#if defined(__linux__) || defined(__APPLE__)
            base::create( internal_dtor );
#else
            base::create();
#endif
        }
        ~tls() { base::destroy(); }
        T* operator=(T* value) { base::set(value); return value; }
        operator T*()   { return  internal_get(); }
        T* operator->() { return  internal_get(); }
        T& operator*()  { return *internal_get(); }
    };
    
} // namespace internal

using namespace internal;

ThreadLocal::ThreadLocal(DestroyFunc pDesFunc)
{
    m_pThreadLocal = new(std::nothrow) ThreadLocalStorage(pDesFunc);
}

ThreadLocal::~ThreadLocal()
{
    ThreadLocalStorage *pTls = (ThreadLocalStorage*)m_pThreadLocal;
    delete pTls;
    m_pThreadLocal = NULL;
}

void ThreadLocal::set(void *value)
{
    ThreadLocalStorage *pTls = (ThreadLocalStorage*)m_pThreadLocal;
    pTls->set(value);
}

void* ThreadLocal::get()
{
    ThreadLocalStorage *pTls = (ThreadLocalStorage*)m_pThreadLocal;
    return pTls->get();
}

NS_BASELIB_END
