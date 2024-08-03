
#include <iostream>
#include <string>
#include <iostream>
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>
#include "const.h"
#include <algorithm>


#include <memory>
#include "CServer.h"
#include "HttpConnection.h"
#include "LogicSystem.h"

#include "ConfigMgr.h"
#include "testDemo.h"
#include "RedisMgr.h"
#include "MysqlDao.h"



int main()
{
    
    printf("\nGateServer start... \n");
    try
    {
        ConfigMgr& configMgr = ConfigMgr::Inst();
        std::string gate_port_str = configMgr["GateServer"]["Port"];
        unsigned short gate_port = atoi(gate_port_str.c_str());
        
        unsigned short port = static_cast<unsigned short>(gate_port);
        boost::asio:: io_context ioc{ 1 };
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
            });
        std::make_shared<CServer>(ioc, port)->Start();
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}