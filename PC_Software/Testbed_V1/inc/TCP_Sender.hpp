/*
 * TCP_Sender.hpp
 *
 *  Created on: 07.06.2019
 *      Author: valtin
 */

#ifndef INC_TCP_Sender_HPP_
#define INC_TCP_Sender_HPP_

#include <cstdio>

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>
//#include <boost/signals2.hpp>

#define _UDP_HANDLE_DEBUG_ 1

using boost::asio::ip::tcp;

enum {  max_length = 65000,
		max_num_sessions = 512
};

class session
{
public:
  session(boost::asio::io_service& io_service);
  tcp::socket& socket(void);
  bool active;

  bool isActive();
  void start(const char* stream, std::size_t msg_length);
  void sendData(const char* stream, std::size_t msg_length);

private:
  tcp::socket socket_;

  void handle_read(const boost::system::error_code& error, size_t bytes_transferred);
  void handle_write(const boost::system::error_code& error);
};


class TCP_Sender {
private:

	boost::asio::io_service* io_service_;
	tcp::acceptor *acceptor_;

	session *session_[max_num_sessions];
	uint16_t session_counter_;

    //Buffer and buffer information
    std::size_t header_length_;
    char header_buffer_[max_length];

    std::size_t data_length_;
    char data_buffer_[max_length];


	void start_accept(void);
	void handle_accept(session* this_session, const boost::system::error_code& error);

public:
    TCP_Sender(unsigned short port);

    void poll(void);
    void stop(void);
    void setHeader(const char*, std::size_t);
    void addData(const char*, std::size_t);
    void sendData(void);
};

#endif /* INC_TCP_Sender_HPP_ */
