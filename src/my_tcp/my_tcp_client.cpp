/**************************************************************
 *  Filename:   my_tcp_client.cpp
 *  Copyright:   jues
 *
 *  Description: my_tcp_client class source file
 *
 *  author: jues
 *  email:  jues1991@163.com
 *  version: 2017-10-30
 **************************************************************/

#include "my_tcp_client.h"

namespace my_socket {
namespace my_tcp {

my_tcp_client::my_tcp_client()
{
    this->m_is_server = false;
    this->m_write = false;
    this->m_write_done = false;
    this->m_started = false;
    this->m_rbuff_size_auto = default_rbuff_size_auto;
    this->m_wbuff_size_auto = default_wbuff_size_auto;
    this->m_rbuff_size = default_rbuff_size;
    this->m_wbuff_size = default_wbuff_size;
    this->m_io = io_service_ptr(new io_service());
    this->m_sock = socket_ptr(new ip::tcp::socket(*this->m_io));

    this->m_ping_timer = timer_ptr(new deadline_timer(*this->m_io));
    this->m_ping_interval = default_ping_interval;
}

my_tcp_client::my_tcp_client( io_service_ptr &io,socket_ptr &sock )
{
    this->m_is_server = true;
    this->m_write = false;
    this->m_write_done = false;
    this->m_started = false;
    this->m_rbuff_size_auto = default_rbuff_size_auto;
    this->m_wbuff_size_auto = default_wbuff_size_auto;
    this->m_rbuff_size = default_rbuff_size;
    this->m_wbuff_size = default_wbuff_size;
    this->m_io = io;
    this->m_sock = sock;

    this->m_ping_timer = timer_ptr(new deadline_timer(*this->m_io));
    this->m_ping_interval = default_ping_interval;
}

my_tcp_client::~my_tcp_client()
{
    this->close();
    this->wait();
}

bool my_tcp_client::start( const endpoint &ep,const size_t &thread_count  )
{
    write_lock lock(this->m_started_mutex);
    //
    if ( true == this->m_started )
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
        //add run post
        this->m_io->post(boost::bind(&my_tcp_client::begin_connect,this,ep));

        //create thread
        this->m_ths = boost::shared_ptr<boost::thread_group>(new boost::thread_group());
        for ( size_t i = 0;thread_count > i ;i++ )
        {
            boost::thread *th = this->m_ths->create_thread(boost::bind(&my_tcp_client::run_thread,this));
            //
            th->detach();
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

void my_tcp_client::stop()
{
    this->m_io->post(boost::bind(&my_tcp_client::close,this));
}

//endpoint
endpoint my_tcp_client::local_endpoint() const
{
    return this->m_sock->local_endpoint();
}

endpoint my_tcp_client::remote_endpoint() const
{
    return this->m_sock->remote_endpoint();
}

//handle_connect
void my_tcp_client::handle_connect( const error_code &error )
{
    read_lock lock(this->m_started_mutex);

    //check stard
    if ( false == this->m_started )
    {
        return;
    }

    //signal
    this->m_connect_sig(error);

    //check error
    if ( error )
    {

        //
        return;
    }
    //ready read
    this->do_read();
}

//handle_disconnect
void my_tcp_client::handle_disconnect( const error_code &error )
{
    read_lock lock(this->m_started_mutex);
    //
    if ( false == this->m_started )
    {
        return;
    }

    //signal
    this->m_disconnect_sig(error);
}

//do_read
void my_tcp_client::do_read()
{
    buff_ptr buf( new vector<char>());
    //
    buf->resize(this->read_buff_size());
    this->m_sock->async_read_some(buffer(*buf),boost::bind(&my_tcp_client::handle_read,this,buf,_1,_2));
    //
    this->ping();
}

//do_write
void my_tcp_client::do_write( buff_ptr &buf )
{
    this->m_sock->async_write_some(buffer(*buf),boost::bind(&my_tcp_client::handle_write,this,buf,_1,_2));
}

void my_tcp_client::handle_read(  buff_ptr &buf,const error_code& error, size_t bytes_transferred  )
{
    if ( error )
    {
#ifdef MY_SOCKET_DEBUG
        cout<<"my_tcp_client::handle_read: error pid="<<boost::this_thread::get_id()<<",error="<<error<<",message="<<error.message()<<endl;
#endif
        if ( boost::system::errc::operation_canceled == error )
        {
            return;
        }
        //if ( boost::asio::error::eof  == error ||
        //     boost::system::errc::connection_reset == error )
        //{
        //}
        this->handle_disconnect(error);
        //
        return;
    }
    //else
    {
        read_lock lock(this->m_started_mutex);
        //
        //auto set read buff size
        size_t available = this->m_sock->available();
        if ( 0 <  available && true == this->auto_read_buff_size() )
        {
            //++
            this->m_rbuff_size += available;
        }

        //signal
        buf->resize(bytes_transferred);
        this->m_read_sig(buf,error,available);


        if ( false == this->m_started )
        {
            return;
        }

        //next
        this->do_read();
    }
}


void my_tcp_client::handle_write( buff_ptr &buf,const error_code& error, size_t bytes_transferred  )
{
    if ( error )
    {
#ifdef MY_SOCKET_DEBUG
        cout<<"my_tcp_client::handle_write: error pid="<<boost::this_thread::get_id()<<",error="<<error<<",message="<<error.message()<<endl;
#endif
        if ( boost::system::errc::operation_canceled == error )
        {
            this->write(false);
            return;
        }
        //if ( boost::asio::error::eof  == error ||
        //     boost::system::errc::connection_reset == error )
        //{
        this->write(false);
        //this->handle_disconnect(error);
        //}

        //
        return;
    }
    //else
    {
        read_lock lock(this->m_started_mutex);
        //
        buf->erase(buf->begin(),buf->begin()+bytes_transferred);
        const size_t size = buf->size();

        //resize wbuff size
        if ( true == this->write_buff_size() )
        {
            if (  0 >= size )
            {
                this->m_wbuff_size += bytes_transferred;
            }
            else
            {
                this->m_wbuff_size = bytes_transferred;
            }
        }

        //check write success
        if ( 0 >= size )
        {
            //signal write
            this->m_write_sig(error,bytes_transferred,size);

            this->write(false);

            //signal write done
            this->write_done(true);
            this->m_write_done_sig(error,size);
            this->write_done(false);

            return;
        }

        //signal write
        this->m_write_sig(error,bytes_transferred,size);


        if ( false == this->m_started )
        {
            this->write(false);
            return;
        }

        //next
        this->do_write(buf);
    }
}


void my_tcp_client::handle_ping( const error_code& error )
{
    if ( error )
    {
        return;
    }

    //
    if ( this->m_ping_timer->expires_at() > deadline_timer::traits_type::now() )
    {
        return;
    }

    //
#ifdef MY_SOCKET_DEBUG
    cout<<"my_tcp_client::handle_ping: time out!!!"<<endl;
#endif
    //error_code ret_error(boost::system::errc::operation_canceled,boost::system::native_ecat);
    //this->handle_disconnect(ret_error);
    //
    if ( nullptr == this->m_sock )
    {
        return;
    }
    this->close();
}


//close
void my_tcp_client::close()
{
    write_lock lock(this->m_started_mutex);
    //
    if ( false == this->m_started )
    {
        return;
    }

    my_tcp_client::close_sock(this->m_sock);
    //this->m_sock.reset(); //use boost::weak_ptr,so don't reset

    if ( false == this->m_is_server )
    {

    }

    //check stard
    this->m_started = false;
}


//close
void my_tcp_client::close_sock(  socket_ptr &sock )
{
    try
    {
        sock->shutdown(ip::tcp::socket::shutdown_both);
        sock->close();
    }
    catch (...) {
    }
}

//write
bool my_tcp_client::write( const vector<char> &buf )
{
    read_lock lock(this->m_started_mutex);
    //
    //check is stated
    if ( false == this->m_started )
    {
        return false;
    }

    //check is write
    if ( true == this->m_write )
    {
        return false;
    }

    //check size
    if ( 0 >= buf.size() )
    {
        return false;
    }
    //
    this->m_write = true;
    //
    buff_ptr buff( new vector<char>(buf));
    //
    this->do_write(buff);
    //
    return true;
}

bool my_tcp_client::write( const char *data,const size_t &size )
{
    vector<char> buf;
    const char *p;
    //
    p = data;
    for ( size_t i = 0;size > i;i++ )
    {
        buf.push_back((*p));
        p++;
    }
    //
    return this->write(buf);
}

bool my_tcp_client::write( const string &str )
{
    vector<char> buf;
    //
    buf.assign(str.begin(),str.end());
    return this->write(buf);
}



void my_tcp_client::write( const bool &state )
{
    this->m_write = state;
    this->m_started_condition.notify_one();
}

void my_tcp_client::write_done( const bool &state )
{
    this->m_write_done = state;
    this->m_started_condition.notify_one();
}

size_t my_tcp_client::ping_interval() const
{
    return this->m_ping_interval;
}

void my_tcp_client::ping_interval( const size_t &interval )
{
    this->m_ping_interval = interval;
}

my_tcp::handle my_tcp_client::handle() const
{
    return this->m_sock->native_handle();
}

void my_tcp_client::read_buff_size( const size_t &size )
{
    if ( 0 >= size )
    {
        this->m_rbuff_size_auto = true;
    }
    else
    {
        this->m_rbuff_size_auto = false;
        this->m_rbuff_size = size;
    }
}

size_t my_tcp_client::read_buff_size()  const
{
    return this->m_rbuff_size;
}

bool my_tcp_client::auto_read_buff_size()  const
{
    return this->m_rbuff_size_auto;
}


void my_tcp_client::write_buff_size( const size_t &size ) // 0 is auto (default)
{
    if ( 0 >= size )
    {
        this->m_wbuff_size_auto = true;
    }
    else
    {
        this->m_wbuff_size_auto = false;
        this->m_wbuff_size = size;
    }
}

size_t my_tcp_client::write_buff_size()  const
{
    return this->m_wbuff_size;
}

bool my_tcp_client::write()  const
{
    return this->m_write;
}

bool my_tcp_client::write_done()  const
{
    return this->m_write_done;
}


void my_tcp_client::wait() const
{
    boost::mutex m;
    boost::mutex::scoped_lock lock(m);
    //
    while ( true == this->write() ||
            true == this->write_done())
    {
        this->m_started_condition.wait(lock);
    }
}

//slots
connection my_tcp_client::add_connect_slot( const boost::function<void (const error_code &error)> &f )
{
    return this->m_connect_sig.connect(f);
}

connection my_tcp_client::add_disconnect_slot( const boost::function<void (const error_code &error)> &f )
{
    return this->m_disconnect_sig.connect(f);
}

connection my_tcp_client::add_read_slot( const boost::function<void (const buff_ptr &buf,const error_code &error,const size_t available)> &f )
{
    return this->m_read_sig.connect(f);
}
connection my_tcp_client::add_write_slot( const boost::function<void (const error_code &error,const size_t transferred,const size_t available)> &f )
{
    return this->m_write_sig.connect(f);
}
connection my_tcp_client::add_write_done_slot( const boost::function<void (const error_code &error,const size_t available)> &f )
{
    return this->m_write_done_sig.connect(f);
}

void my_tcp_client::started( const bool &state )
{
    write_lock lock(this->m_started_mutex);
    //
    this->m_started = state;
}

bool my_tcp_client::started() const
{
    read_lock lock(this->m_started_mutex);
    //
    return this->m_started;
}


//run_thread
void my_tcp_client::run_thread()
{
    this->m_io->run();
}

void my_tcp_client::begin_connect( const endpoint &ep )
{
    this->m_sock->async_connect(ep,boost::bind(&my_tcp_client::handle_connect,this,_1));
}


void my_tcp_client::ping()
{
    if ( 0 >= this->m_ping_interval )
    {
        return; //disable
    }
    //
    this->m_ping_timer->expires_from_now(boost::posix_time::milliseconds(this->m_ping_interval));
    this->m_ping_timer->async_wait(boost::bind(&my_tcp_client::handle_ping,this,_1));
}


} //my_tcp
} //my_socket
