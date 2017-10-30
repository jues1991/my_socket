/**************************************************************
 *  Filename:   main.cpp
 *  Copyright:   jues
 *
 *  Description: my_tcp_demo demo source file.
 *
 *  author: jues
 *  email:  jues1991@163.com
 *  version: 2017-10-30
 **************************************************************/

#include <iostream>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include "my_tcp.h"

using namespace std;
using namespace my_socket::my_tcp;

namespace po = boost::program_options;


static bool g_is_reply = false;

//client_read
static void client_read(client_wptr &wclient,const buff_ptr &buf,const my_socket::my_tcp::error_code &error,const size_t available)
{
    try {
        client_ptr client = wclient.lock();
        if ( nullptr == client )
        {
            return; //invalid
        }
        //
        if ( error )
        {
            cout<<"client_read: client="<<client->local_endpoint()<<",error="<<error<<",message="<<error.message()<<",available="<<available<<endl;
        }
        else
        {
            cout<<"client_read: client="<<client->local_endpoint()<<",size="<<buf->size()<<",available="<<available<<endl;
        }

        //is reply
        if ( true == g_is_reply )
        {
            client->write(*buf);
        }
    }
    catch( const exception &e )
    {
        cout<<"client_read: "<<e.what()<<endl;
    }

}

//server_accept
static void server_accept(client_wptr &wclient,const my_socket::my_tcp::error_code &error)
{
    try {
        client_ptr client = wclient.lock();
        if ( nullptr == client )
        {
            return; //invalid
        }
        //
        cout<<"accept: "<<client->local_endpoint()<<endl;

        //slot
        client->add_read_slot(boost::bind(client_read,wclient,_1,_2,_3));

    }
    catch( const exception &e )
    {
        cout<<"accept: "<<e.what()<<endl;
    }
}







int main(int argc, char *argv[])
{
    po::options_description desc("my_tcp_demo options");
    string ip;
    unsigned int port;
    size_t thread_count;
    desc.add_options()
            ("help,h", "show help.")
            ("ip,i", po::value<string>(&ip)->default_value("0.0.0.0"), "server bind ip.")
            ("port,p", po::value<unsigned int>(&port)->default_value(0), "server bind port.")
            ("thread,t", po::value<size_t>(&thread_count)->default_value(4), "work thread count.")
            ("reply,r", "server to reply the received data.")
            ;
    po::positional_options_description p;
    p.add("port", -1);


    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);


    //check param
    if ( vm.count("help") )
    {
        cout << desc << endl;
        return 1;
    }

    //reply
    if ( vm.count("reply") )
    {
        g_is_reply = true;
    }


    //server
    my_tcp_server tcp;
    my_socket::my_tcp::error_code error;

    //slot
    tcp.add_accept_slot(server_accept);

    //bind
    if ( (error=tcp.bind(ip,port) ) )
    {
        cout <<"bind: error="<<error<<",message"<<error.message()<< endl;
        return 2;
    }
    cout <<"bind: "<<tcp.local_endpoint()<< endl;

    //start
    if ( false == tcp.start(thread_count) )
    {
        cout <<"start: faill!!!"<< endl;
        return 3;
    }

    return 0;
}
