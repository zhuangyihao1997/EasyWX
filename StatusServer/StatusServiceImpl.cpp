#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "const.h"
#include "RedisMgr.h"


std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);
    return unique_string;
}
Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{ 
    std::cout << "llfc status server has received : " << std::endl;
    std::string prefix("llfc status server has received :  ");
    _server_index += 1;
    int _server_chose_index = _server_index % (_servers.size());
    std::cout << "server_index:" <<_server_index<< std::endl;
    auto& server = _servers[_server_chose_index];
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(ErrorCodes::Success);
    reply->set_token(generate_unique_string());

    std::cout << "insertToken " << std::endl;
    insertToken(request->uid(), reply->token());
    std::cout << "insertToken  sunncess" << std::endl;

    return Status::OK;
}
StatusServiceImpl::StatusServiceImpl() :_server_index(0)
{
    auto& cfg = ConfigMgr::Inst();
    ChatServer server;
    server.port = cfg["ChatServer1"]["Port"];
    server.host = cfg["ChatServer1"]["Host"];
    _servers.push_back(server);
    server.port = cfg["ChatServer2"]["Port"];
    server.host = cfg["ChatServer2"]["Host"];
    _servers.push_back(server);
}

void StatusServiceImpl::insertToken(int uid, std::string token)
{
    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;  //_Utoken+uid
    RedisMgr::GetInstance()->Set(token_key, token);
}

Status  StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) {

    auto uid = request->uid();
    auto token = request->token();

    std::string uid_str = std::to_string(uid);
    std::string token_key = USERTOKENPREFIX + uid_str;
    std::string token_value = "";
    bool success = RedisMgr::GetInstance()->Get(token_key, token_value);
    if (success) {
        reply->set_error(ErrorCodes::UidInvalid);
        return Status::OK;
    }

    if (token_value != token) {
        reply->set_error(ErrorCodes::TokenInvalid);
        return Status::OK;
    }
    reply->set_error(ErrorCodes::Success);
    reply->set_uid(uid);
    reply->set_token(token);
    return Status::OK;

}
