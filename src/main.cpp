#include "scriptprocessing.h"
#include "serial.h"

#include <iostream>
#include <vector>
#include <boost/program_options.hpp>

int main(int argc, char *argv[])
{
    std::string slavePort;
    std::string commandFile;
    size_t baudRate;

    boost::program_options::options_description desc("Required options");
    desc.add_options()("help,h", "Show help")("port,p", boost::program_options::value<std::string>(&slavePort)->required(), "Slave serial port")("baud,b", boost::program_options::value<size_t>(&baudRate)->required(), "Baud rate")("command file,f", boost::program_options::value<std::string>(&commandFile), "The path to command file");

    boost::program_options::variables_map vm;
    try
    {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            std::cout << desc << "\n";
            return 0;
        }

        boost::program_options::notify(vm);
    }
    catch (const boost::program_options::required_option &e)
    {
        std::cerr << "Missing required argument: " << e.get_option_name() << "\n";
        std::cerr << desc << "\n";
        return -1;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing argument(s): " << e.what() << "\n";
        std::cerr << desc << "\n";
        return -1;
    }

    CommandBuffer buffer;
    if (!parseCommandFile(commandFile, buffer))
        return -1;

    if (!sendBuffer(buffer, slavePort, baudRate))
        return -1;

    return 0;
}
