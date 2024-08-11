#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
 using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::StatusService;

using message::LoginReq;
using message::LoginRsp;


struct ChatServer {
    std::string host;
    std::string port;
};
class StatusServiceImpl : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
        GetChatServerRsp* reply) override;
    std::vector<ChatServer> _servers;


    void insertToken(int uid, std::string token);
    Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply);

    int _server_index;
};

