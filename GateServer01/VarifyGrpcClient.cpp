#include "VarifyGrpcClient.h"


RPConPool::RPConPool(std::size_t poolSize, std::string host, std::string port)
	:poolSize_(poolSize),host_(host),port_(port),b_stop_(false)
{
	for (size_t i = 0; i < poolSize_; i++) {
		std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port, grpc::InsecureChannelCredentials());
		connections_.push(VarifyService::NewStub(channel));
	}
}

RPConPool::~RPConPool() {
	std::lock_guard<std::mutex> lock(mutex_);
	Close();
	while(!connections_.empty()){
		connections_.pop();
	}
}

void RPConPool::Close() {

	b_stop_ = true;
	cond_.notify_all();
}

std::unique_ptr<message::VarifyService::Stub > RPConPool::getConnection() {

	std::unique_lock<std::mutex> m_ul(mutex_);
	cond_.wait(m_ul, [this]() {
		return b_stop_.load() || !connections_.empty();
		});
	if (b_stop_.load()) {
		return nullptr;
	}
	std::unique_ptr<message::VarifyService::Stub> connection = std::move(connections_.front());
	connections_.pop();
	return connection;

}

void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub> context) {
	std::lock_guard<std::mutex> lock(mutex_);
	if (b_stop_) {
		return;
	}
	connections_.push(std::move(context));
	cond_.notify_one();
}