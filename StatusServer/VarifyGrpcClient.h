#pragma once

#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPConPool {

public:
	RPConPool(std::size_t poolSize, std::string host, std::string port);
	~RPConPool();
	void Close();
	std::unique_ptr< message::VarifyService::Stub > getConnection();
	void returnConnection(std::unique_ptr<VarifyService::Stub> context);
private:
	std::atomic<bool> b_stop_;
	std::size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<message::VarifyService::Stub>>  connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

class VarifyGrpcClient : public Singleton<VarifyGrpcClient>
{
	friend class Singleton<VarifyGrpcClient>;
public:
	GetVarifyRsp GetVarifyCode(std::string email) {
		ClientContext context;
		GetVarifyRsp reply;
		GetVarifyReq request;
		request.set_email(email);

		auto stub = _pool->getConnection();

		Status status = stub->GetVarifyCode(&context, request, &reply);

		if (status.ok()) {
			_pool->returnConnection(std::move(stub));
			return reply;
		}
		else {
			_pool->returnConnection(std::move(stub));
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}

private:
	std::unique_ptr<VarifyService::Stub> stub_;
	VarifyGrpcClient()   {
		//std::shared_ptr<Channel> channel = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
		//stub_ = VarifyService::NewStub(channel);
		auto& gconfigMgr = ConfigMgr::Inst();
		std::string host = gconfigMgr["VarifyServer"]["Host"];
		std::string port = gconfigMgr["VarifyServer"]["Port"];

		_pool.reset(new RPConPool(5, host, port));

	}
	std::unique_ptr<RPConPool> _pool;

};

