/**************************************************************
 *  Filename:   my_tcp_client.h
 *  Copyright:   jues
 *
 *  Description: my_tcp_client class head file
 *
 *  author: jues
 *  email:  jues1991@163.com
 *  version: 2017-10-30
 **************************************************************/


#ifndef MY_TCP_CLIENT_H
#define MY_TCP_CLIENT_H
#include "my_tcp_def.h"


namespace my_socket {
namespace my_tcp {
// MY_SOCKET_DEBUG

using namespace std;
using namespace boost::asio;

const bool default_rbuff_size_auto = true;
const bool default_wbuff_size_auto = false;

const size_t default_rbuff_size = 1*1024; //byte
const size_t default_wbuff_size = 1*1024; //byte

const size_t default_ping_interval =  1*60*1000; //ms


class my_tcp_client;


typedef ip::tcp::endpoint endpoint;
typedef boost::shared_ptr<vector<char> > buff_ptr;
typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
typedef boost::shared_ptr<my_tcp_client> client_ptr;
typedef boost::shared_ptr<io_service> io_service_ptr;
typedef boost::shared_ptr<deadline_timer> timer_ptr;

typedef boost::weak_ptr<ip::tcp::socket> socket_wptr;
typedef boost::weak_ptr<my_tcp_client> client_wptr;


typedef boost::system::error_code error_code;

typedef boost::shared_lock<boost::shared_mutex> read_lock;
typedef boost::unique_lock<boost::shared_mutex> write_lock;

typedef boost::signals2::connection connection;
typedef ip::tcp::socket::native_handle_type handle;

typedef boost::atomic_bool a_bool;
typedef boost::atomic_size_t a_size_t;


class my_tcp_client : public enable_shared_from_this<my_tcp_client>,public boost::noncopyable
{
public:
    explicit my_tcp_client();
    ~my_tcp_client();
public:
    bool start( const endpoint &ep,const size_t &thread_count = 1 );
    void stop();

    //endpoint
    endpoint local_endpoint() const;
    endpoint remote_endpoint() const;

    //write
    bool write( const vector<char> &buf );
    bool write( const char *data,const size_t &size );
    bool write( const string &str );

    //ping
    size_t ping_interval() const;
    void ping_interval( const size_t &interval );

    //
    bool started() const;
public:
    //handle
    my_tcp::handle handle() const;

    //read buff size
    void read_buff_size( const size_t &size ); // 0 is auto (default)
    size_t read_buff_size() const;
    bool auto_read_buff_size() const;


    //write buff size
    void write_buff_size( const size_t &size ); // 0 is auto (default)
    size_t write_buff_size()  const;
    bool write()  const; //is write working
    bool write_done()  const; //is write working

    void wait() const;
public:
    //slot
    connection add_connect_slot( const boost::function<void (const error_code &error)> &f );
    connection add_disconnect_slot( const boost::function<void (const error_code &error)> &f );
    connection add_read_slot( const boost::function<void (const buff_ptr &buf,const error_code &error,const size_t available)> &f );
    connection add_write_slot( const boost::function<void (const error_code &error,const size_t transferred,const size_t available)> &f );
    connection add_write_done_slot( const boost::function<void (const error_code &error,const size_t available)> &f );
protected:
    void handle_connect( const error_code &error );
    void handle_disconnect( const error_code &error );
    void handle_read( buff_ptr &buf,const error_code& error, size_t bytes_transferred  );
    void handle_write( buff_ptr &buf,const error_code& error, size_t bytes_transferred  );
    void handle_ping( const error_code& error );


    void do_read();
    void do_write( buff_ptr &buf );

    void write( const bool &state );
    void write_done( const bool &state );
    void started( const bool &state );

    void close();
    void run_thread();
    void begin_connect( const endpoint &ep );

    void ping();
protected:
    static void close_sock(  socket_ptr &sock );
protected:
    explicit my_tcp_client( io_service_ptr &io,socket_ptr &sock );
protected:
    //
    bool m_is_server;
    //
    io_service_ptr m_io;
    socket_ptr m_sock;

    timer_ptr m_ping_timer;
    size_t m_ping_interval;

    boost::shared_ptr<boost::thread_group> m_ths;

    //buff
    bool m_rbuff_size_auto;
    size_t m_rbuff_size;


    bool m_write;
    bool m_wbuff_size_auto;
    size_t m_wbuff_size;
    bool m_write_done;

    bool m_started;
    mutable boost::shared_mutex m_started_mutex;
    mutable boost::condition_variable m_started_condition;
    //signal
    boost::signals2::signal<void (const error_code &error)> m_connect_sig;
    boost::signals2::signal<void (const error_code &error)> m_disconnect_sig;
    boost::signals2::signal<void (const buff_ptr &buf,const error_code &error,const size_t available)> m_read_sig;
    boost::signals2::signal<void (const error_code &error,const size_t transferred,const size_t available)> m_write_sig;
    boost::signals2::signal<void (const error_code &error,const size_t available)> m_write_done_sig;
    //
    friend class my_tcp_server;
};


} //my_tcp
} //my_socket

#endif // MY_TCP_CLIENT_H
