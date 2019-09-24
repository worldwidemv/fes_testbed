/*
 * TCP_Sender.cpp
 *
 *  Created on: 07.06.2019
 *      Author: valtin
 */

#include "TCP_Sender.hpp"

using boost::asio::ip::tcp;

session::session(boost::asio::io_service& io_service)
: socket_(io_service)
{
	this->active = false;
}

boost::asio::ip::tcp::socket& session::socket()
{
	return socket_;
}

bool session::isActive()
{
	return this->active;
}

void session::start(const char* stream, std::size_t msg_length)
{
	// send header data to the client
	if (this->socket_.is_open()){
		this->active = true;
		try {
			boost::asio::write(this->socket_, boost::asio::buffer(stream, msg_length));
		}
		catch (std::exception& e)
		{
			// lost connection
			this->active = false;
			std::cerr << "\n\n   ### TCP connection closed ... ###\n\n";
			//std::cerr << "Exception ###: " << e.what() << "\n";
		}
	}

}

void session::sendData(const char* stream, std::size_t msg_length)
{
	// send the current data to the client
	try {
		boost::asio::write(this->socket_, boost::asio::buffer(stream, msg_length));
	}
	catch (std::exception& e)
	{
		// lost connection
		this->active = false;
		std::cerr << "\n\n   ### TCP connection closed ... ###\n\n";
		//std::cerr << "Exception ###: " << e.what() << "\n";
	}
}

void session::handle_read(const boost::system::error_code& error,
		size_t bytes_transferred)
{
	printf("\nSession read handle ...\n");
}

void session::handle_write(const boost::system::error_code& error)
{
	printf("\nSession write handle ...\n");
}





/**
    This class is used to create a socket for a given local port.
    A recieved package can be send handle by another function by binding such a function to
    this->otherHandleRecieve

    @param local_port  Local port
    @param remote_port Remote port
    @param local_host  Local network interface
    @param remote_host Remote/Target interface
 */
TCP_Sender::TCP_Sender(unsigned short port)
{
	this->io_service_ = new boost::asio::io_service();
	this->acceptor_ = new tcp::acceptor(*this->io_service_, tcp::endpoint(tcp::v4(), port));

	this->session_counter_ = 0;
	this->header_length_ = 0;
	this->data_length_ = 0;

	// listen for new connections
	this->start_accept();
}

void TCP_Sender::start_accept(void)
{
	this->session_[this->session_counter_] = new session(*this->io_service_);
	this->acceptor_->async_accept(this->session_[this->session_counter_]->socket(),
			boost::bind(&TCP_Sender::handle_accept, this, this->session_[this->session_counter_],
					boost::asio::placeholders::error));
}

void TCP_Sender::handle_accept(session* this_session, const boost::system::error_code& error)
{
	if (!error) {
		// no error -> start session
		this_session->start(this->header_buffer_, this->header_length_);
		this->session_counter_++;
		printf("\n\n   ### TCP connection established ... ###\n\n");
	} else {
		// error -> delete this session
		this->session_counter_--;
		delete this_session;
	}

	// listen for new connections
	this->start_accept();
}





/**
    Public function called to write to the internal header buffer.

    @param stream Data stream which should be sent.
    @param msg_length Defines the amount of elements in the data stream
 */
void TCP_Sender::setHeader(const char* stream, std::size_t msg_length) {
	this->header_length_ = msg_length *sizeof(char);
	std::memcpy(this->header_buffer_, stream, sizeof(char)*msg_length);
}


/**
    Public function called to write to the internal data buffer.

    @param stream Data stream which should be sent.
    @param msg_length Defines the amount of elements in the data stream
 */
void TCP_Sender::addData(const char* stream, std::size_t msg_length)
{
	if (this->session_counter_ == 0) {
		return;
	}

	std::memcpy(&this->data_buffer_[this->data_length_], stream, sizeof(char)*msg_length);
	this->data_length_ += msg_length *sizeof(char);
}

/**
    Public function called to write to the internal data buffer.

    @param stream Data stream which should be sent.
    @param msg_length Defines the amount of elements in the data stream
 */
void TCP_Sender::sendData()
{
	if (this->session_counter_ == 0 && this->data_length_ == 0) {
		return;
	}

	for (uint16_t i=0; i<this->session_counter_; i++){
		if (this->session_[i] != NULL){
			if (this->session_[i]->isActive()){
				this->session_[i]->sendData(this->data_buffer_, this->data_length_);
			}
		}
	}
	this->data_length_ = 0;

	// run the io_service callbacks
	this->io_service_->poll();
}

/**
    This function must be called to start the tcp server and listen for connections
 */
void TCP_Sender::poll()
{
	// start the execution
	this->io_service_->poll();
}

/**
    This function must be called to stop the udp server and join the started thread.
 */
void TCP_Sender::stop() {

}
