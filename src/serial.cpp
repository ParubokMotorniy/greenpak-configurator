#include "serial.h"

#include <boost/asio.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <atomic>
#include <filesystem>
#include <iostream>
#include <thread>

bool sendBuffer(const CommandBuffer &buffer, const std::string &port, size_t baudRate)
{
    if (!std::filesystem::exists(port))
        return false;

    boost::asio::io_service io;
    boost::asio::serial_port communicator(io, port);
    boost::asio::deadline_timer timeout(io);

    if (!communicator.is_open())
        return false;

    communicator.set_option(boost::asio::serial_port_base::baud_rate(baudRate));
    communicator.set_option(
        boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    communicator.set_option(
        boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    communicator.set_option(boost::asio::serial_port_base::character_size(8u));
    communicator.set_option(boost::asio::serial_port_base::flow_control(
        boost::asio::serial_port_base::flow_control::none));

    try
    {
        for (const Protocol::CommandRepresentation command : buffer)
        {
            uint8_t ackByte = Protocol::FlowControl::NACK;

            do
            {
                if (ackByte != Protocol::FULL)
                {
                    // no need to retransmit if the slave if simply full...
                    boost::asio::write(communicator, boost::asio::buffer(&command, 2));
                    std::cout << "Sent a command to slave: " << command << std::endl;
                }

                std::atomic<bool> timedOut = false;
                boost::system::error_code readError;

                timeout.expires_from_now(boost::posix_time::milliseconds(5000));
                timeout.async_wait([&](const boost::system::error_code &ec) {
                    if (!ec)
                    {
                        timedOut = true;
                        communicator.cancel();
                    }
                });

                boost::asio::async_read(communicator, boost::asio::buffer(&ackByte, 1),
                                        [&](const boost::system::error_code &ec, std::size_t) {
                                            readError = ec;
                                            timeout.cancel();
                                        });

                io.reset();
                io.run();

                if (timedOut || readError)
                    ackByte = Protocol::FlowControl::NACK;

                switch (ackByte)
                {
                case Protocol::FlowControl::ACK:
                    std::cout << "Received ACK!" << std::endl;
                    break;
                case Protocol::FlowControl::NACK:
                    std::cout << "Received NACK!" << std::endl;
                    break;
                case Protocol::FlowControl::FULL:
                    std::cout << "Received FULL!" << std::endl;
                    break;
                default:
                    std::cout << "Unknown response" << std::endl;
                    ackByte = Protocol::FlowControl::NACK;
                    break;
                }

            } while (ackByte != Protocol::FlowControl::ACK);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Some boost::asio error. Details: " << e.what() << std::endl;
        return false;
    }

    return true;
}
