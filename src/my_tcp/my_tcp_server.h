/**************************************************************
 *  Filename:   my_tcp_server.h
 *  Copyright:   jues
 *
 *  Description: my_tcp_server class head file
 *
 *  author: jues
 *  email:  jues1991@163.com
 *  version: 2017-10-30
 **************************************************************/


#ifndef MY_TCP_SERVER_H
#define MY_TCP_SERVER_H

#include "my_tcp_client.h"

namespace my_socket {
namespace my_tcp {

class my_tcp_server
{
public:
    my_tcp_server();

public:
    error_code bind( const unsigned int &port );
    error_code bind( const string &ip,const unsigned int &port );
    error_code bind( const ip::tcp::endpoint &ep );
    error_code bind( const endpoint &ep, error_code &error );

    endpoint local_endpoint() const;

public:
    bool start( const size_t &thread_count = 1,const bool &detach = false );
    void stop();

    bool started() const;
public:
    //slot
     connection add_accept_slot( const boost::function<void (client_wptr &client,const error_code &error)> &f );
protected:
    void run_thread();
    void start_accept();

    void handle_accept( socket_ptr &sock,const error_code &error );

    //slots
    void handle_disconnect( client_wptr &wclient,const error_code &error );
    void remove_client( client_wptr &wclient );


    void started( const bool &state );
protected:
    ip::tcp::endpoint m_ep;
    io_service_ptr m_io;
    boost::shared_ptr<ip::tcp::acceptor> m_acc;
    boost::shared_ptr<boost::thread_group> m_ths;

    //started
    bool m_bind;
    bool m_started;
    mutable boost::shared_mutex m_started_mutex;

    //client
    set<client_ptr> m_cs;
    boost::shared_mutex m_cs_mutex;

    //accept
    bool m_is_accept;
    boost::shared_mutex m_accept_mutex;

    //signal
    boost::signals2::signal<void (client_wptr &client,const error_code &error)> m_accept_sig;
};




} //my_tcp
} //my_socket
#endif // MY_TCP_SERVER_H
