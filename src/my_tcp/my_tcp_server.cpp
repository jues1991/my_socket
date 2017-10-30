/**************************************************************
 *  Filename:   my_tcp_server.h
 *  Copyright:   jues
 *
 *  Description: my_tcp_server class source file
 *
 *  author: jues
 *  email:  jues1991@163.com
 *  version: 2017-10-30
 **************************************************************/


#include "my_tcp_server.h"

namespace my_socket {
namespace my_tcp {




my_tcp_server::my_tcp_server()
{
    this->m_io = io_service_ptr(new io_service());
    this->m_acc = boost::shared_ptr<ip::tcp::acceptor>(new ip::tcp::acceptor(*this->m_io));
    //this->m_ths = boost::shared_ptr<boost::thread_group>(new boost::thread_group());
}


error_code my_tcp_server::bind( const unsigned int &port )
{
    return this->bind("0.0.0.0",port);
}

error_code my_tcp_server::bind( const string &ip,const unsigned int &port )
{
    endpoint ep( ip::address::from_string(ip),port );
    //
    return this->bind(ep);
}

error_code my_tcp_server::bind( const endpoint &ep )
{
    error_code error;
    //
    return this->bind(ep,error);
}

error_code my_tcp_server::bind( const endpoint &ep, error_code &error )
{
    write_lock lock(this->m_started_mutex);
    error_code ret_error;
    //
    this->m_acc->open(ep.protocol());
    this->m_acc->set_option(ip::tcp::acceptor::reuse_address(true));
    //
    ret_error = this->m_acc->bind(ep,error);
    if ( !ret_error )
    {
        this->m_bind = true;
    }
    else
    {
        this->m_bind = false;
    }

    //
    return ret_error;
}

endpoint my_tcp_server::local_endpoint() const
{
    return this->m_acc->local_endpoint();
}

bool my_tcp_server::start( const size_t &thread_count,const bool &detach )
{
    write_lock lock(this->m_started_mutex);

    //check bind
    if ( false == this->m_bind )
    {
        return false;
    }

    //check param
    if ( 0 >= thread_count )
    {
        return false;
    }

    //
    try{
        //acceptor
        this->m_acc->listen();

        //add run post
        this->m_io->post(boost::bind(&my_tcp_server::start_accept,this));

        //create thread
        this->m_ths = boost::shared_ptr<boost::thread_group>(new boost::thread_group());
        for ( size_t i = 0;thread_count > i ;i++ )
        {
            boost::thread *th = this->m_ths->create_thread(boost::bind(&my_tcp_server::run_thread,this));
            //
            if ( true == detach )
            {
                th->detach();
            }
        }
        //
        if ( false == detach )
        {
            this->m_ths->join_all();
        }
    }
    catch (...)
    {
        return false;
    }

    //
    this->m_started = true;
    return true;
}


void my_tcp_server::stop()
{
    this->m_acc->close();
    this->started(false);
}

bool my_tcp_server::started() const
{
    read_lock lock(this->m_started_mutex);
    //
    return this->m_started;
}

void my_tcp_server::started( const bool &state )
{
    write_lock lock(this->m_started_mutex);
    //
    this->m_started = state;
}


//run_thread
void my_tcp_server::run_thread()
{
    this->m_io->run();
}

//start_accept
void my_tcp_server::start_accept()
{
    write_lock lock(this->m_accept_mutex);
    socket_ptr sock(new ip::tcp::socket(*this->m_io));
    //
    this->m_is_accept = false;

    //accept
    this->m_acc->async_accept(*sock,boost::bind(&my_tcp_server::handle_accept,this,sock,_1));
}


//handle_accept
void my_tcp_server::handle_accept( socket_ptr &sock,const error_code &error  )
{
    if ( error )
    {
#ifdef MY_SOCKET_DEBUG
        cout<<"my_tcp_server::handle_accept: error pid="<<boost::this_thread::get_id()<<",error="<<error<<",message="<<error.message()<<endl;
#endif
        //accept
        if ( boost::system::errc::too_many_files_open == error )
        {//boost::asio::error::no_descriptors
            write_lock lock(this->m_accept_mutex);
            //
            this->m_is_accept = true;
        }

        //
        return;
    }

    {
        //new client
        client_ptr client( new my_tcp_client(this->m_io,sock));
        client_wptr wclient(client);

        //save client
        {
            write_lock lock(this->m_cs_mutex);
            //
            this->m_cs.insert(client);
            client->add_disconnect_slot(boost::bind(&my_tcp_server::handle_disconnect,this,wclient,_1));
        }

        //accept signal
        this->m_accept_sig(wclient,error);

        //client start
        client->started(true);
        client->handle_connect(error);
    }



    //next
    this->start_accept();
}


//slot
connection my_tcp_server::add_accept_slot( const boost::function<void (client_wptr &client,const error_code &error)> &f )
{
    return this->m_accept_sig.connect(f);
}


//slots
void my_tcp_server::handle_disconnect( client_wptr &wclient,const error_code &error )
{
    //add remove client run post
    this->m_io->post(boost::bind(&my_tcp_server::remove_client,this,wclient));
}




void my_tcp_server::remove_client( client_wptr &wclient )
{
    client_ptr client = wclient.lock();
    //
    if ( nullptr == client )
    {
        return;
    }

    //wait exit
    //client->wait();

    //remove
    {
        write_lock lock(this->m_cs_mutex);
        //
        this->m_cs.erase(client);
    }


    {
        //accept
        read_lock lock(this->m_accept_mutex);
        //
        if ( false == this->m_is_accept )
        {
            return;
        }
    }
    //next
    this->start_accept();
}



} //my_tcp
} //my_socket
