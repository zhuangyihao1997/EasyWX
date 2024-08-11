#include "HttpConnection.h"
#include <iostream>

#include <string>
#include <cassert>

 unsigned char ToHex(unsigned char x)
{
    return  x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x)
{
    unsigned char y;
    if (x >= 'A' && x <= 'Z') y = x - 'A' + 10;
    else if (x >= 'a' && x <= 'z') y = x - 'a' + 10;
    else if (x >= '0' && x <= '9') y = x - '0';
    else assert(0);
    return y;
}

std::string UrlEncode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //判断是否仅有数字和字母构成
        if (isalnum((unsigned char)str[i]) ||
            (str[i] == '-') ||
            (str[i] == '_') ||
            (str[i] == '.') ||
            (str[i] == '~'))
            strTemp += str[i];
        else if (str[i] == ' ') //为空字符
            strTemp += "+";
        else
        {
            //其他字符需要提前加%并且高四位和低四位分别转为16进制
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str)
{
    std::string strTemp = "";
    size_t length = str.length();
    for (size_t i = 0; i < length; i++)
    {
        //还原+为空
        if (str[i] == '+') strTemp += ' ';
        //遇到%将后面的两个字符从16进制转为char再拼接
        else if (str[i] == '%')
        {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        }
        else strTemp += str[i];
    }
    return strTemp;
}


HttpConnection::HttpConnection(tcp::socket socket):_socket(std::move(socket)) {

}

HttpConnection::HttpConnection(boost::asio::io_context& ioc):_socket(ioc)
{

}

/*******************************
考虑在HttpConnection::Start内部调用http::async_read函数，其源码为
async_read(
    AsyncReadStream& stream,
    DynamicBuffer& buffer,
    basic_parser<isRequest>& parser,
    ReadHandler&& handler)
********************************/


void HttpConnection::Start() 
{
    auto self = shared_from_this();
    http::async_read(_socket, _buffer, _request, [self,this](beast::error_code ec,std::size_t bytes_transferred) 
        {
            try
            {   
                /*std::string requestPostUrl(_request.target().begin(), _request.target().end());
                std::cout << "Post request is " << requestPostUrl << std::endl;*/
                if (ec) {
                    if (ec == boost::beast::http::error::end_of_stream) 
                   {
                        std::cout << "http read err :" << ec.what() << std::endl;
                    }
                    else {
                        std::cout << "http read err : " << ec.what() << std::endl;
                        std::cout << "return" << std::endl;
                        return;
                    }
                    
                }
                //处理读到的数据
                boost::ignore_unused(bytes_transferred);
                self->HandleReq();
                self->CheckDeadline();

            }
            catch (const std::exception& e)
            {
                std::cout << "exception err is " << e.what() << std::endl;
            }
        });
}

void HttpConnection::HandleReq()
{
    //设置版本
    _response.version(_request.version());

    //设置短连接
    _response.keep_alive(false);

    if (_request.method() == http::verb::get) 
    {   
        std::cout << "resuest is :" << _request.target() << std::endl;
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;

    }
    else if (_request.method() == http::verb::post)
    {
        std::string requestPostUrl(_request.target().begin(), _request.target().end());
        std::cout << "Post request is " << requestPostUrl << std::endl;
        bool success = LogicSystem::GetInstance()->HandlePost(requestPostUrl, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }
        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::WriteResponse()
{
    auto self = shared_from_this();
    _response.content_length(_response.body().size());
    http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);
        self->deadline_.cancel();
        });
}

void HttpConnection::CheckDeadline()
{
    auto self = shared_from_this();
    deadline_.async_wait(
        [self](beast::error_code ec) {
            if (!ec) {
                self->_socket.close(ec);
            }
        }
    );
}

void HttpConnection::PreParseGetParam()
{
    // 提取 URI  
    auto target = _request.target();
    // 查找查询字符串的开始位置（即 '?' 的位置）  

    std::string uri(target.begin(),target.end());
    auto query_pos = uri.find('?');
   
    if (query_pos == std::string::npos) {
        _get_url.clear();
        _get_url.assign(uri.begin(),uri.end());

        std::cout << "_get_url is " << _get_url << std::endl;

        return;
    }

    _get_url = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;

    size_t pos = 0;

    while ((pos = query_string.find('&')) != std::string::npos) 
    {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos)); // 假设有 url_decode 函数来处理URL解码  
            value = UrlDecode(pair.substr(eq_pos + 1));
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);

    }
    // 处理最后一个参数对（如果没有 & 分隔符）
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}


