#pragma once

#include "const.h"
#include <memory>

#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include "LogicSystem.h"
#include <unordered_map>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    HttpConnection(boost::asio::io_context& ioc);
    void Start();
    tcp::socket& GetSocket() { return _socket; }

private:

    void HandleReq();
    void WriteResponse();
    void CheckDeadline();
    void PreParseGetParam();
    

    tcp::socket _socket;

    //��������
    beast::flat_buffer _buffer{ 8192 };
    //��������
    beast::http::request<http::dynamic_body> _request;
    //�ͻ�����Ӧ
    beast::http::response<http::dynamic_body> _response;
    //��ʱ�� �жϳ�ʱ
    boost::asio::steady_timer deadline_{_socket.get_executor(),std::chrono::seconds(50)};
    //get �����������
    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;

};
