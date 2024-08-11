#include "RedisMgr.h"
#include "ConfigMgr.h"

RedisPool::RedisPool(std::size_t poolSize, const char* host, int port, const char* pwd) :_poolSize(poolSize), _host(host), _port(port), _pwd(pwd), b_stop(false)
{
	for (size_t i = 0; i < _poolSize; i++) {
		auto* context = redisConnect(_host, _port);
		if (context == nullptr || context->err != 0) {
			if (context != nullptr) {
				redisFree(context);
			}
			continue;
		}
		auto* reply = (redisReply*)redisCommand(context, "AUTH %s",_pwd);
		if (reply->type == REDIS_REPLY_ERROR) {
			std::cout << "��֤ʧ��" << std::endl;
			//ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
			freeReplyObject(reply);
			continue;
		}
		//ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
		freeReplyObject(reply);
		std::cout << "��֤�ɹ�" << std::endl;
		connections_.push(context);

	}
}

RedisPool::~RedisPool() {
	std::lock_guard<std::mutex> lock(_mutex);
	while (!connections_.empty()) {
		connections_.pop();
	}
}

redisContext* RedisPool:: getConnection() {
	std::unique_lock<std::mutex> lock(_mutex);
	_cv.wait(lock, [this]() {
		if (b_stop) {
			return true;
		}
		return !connections_.empty();
		});
	if (b_stop) {
		return nullptr;
	}
	auto* context = connections_.front();
	connections_.pop();
	return context;
}

void RedisPool:: returnConnection(redisContext* context) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (b_stop) {
		return;
	}
	connections_.push(context);
	_cv.notify_one();
}

void RedisPool::close() {
	b_stop = true;
	_cv.notify_all();
}


bool RedisMgr::Connect(const std::string& host, int port)
{	
	auto* connect = _redisPool->getConnection();
	
	if (connect != NULL && connect->err)
	{
		std::cout << "connect error " << connect->errstr << std::endl;
		return false;
	}
	return true;
}
bool RedisMgr::Get(const std::string& key, std::string& value)
{
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "GET %s", key.c_str());
	if (reply == NULL) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_STRING) {
		std::cout << "[ GET  " << key << " ] failed" << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	value = reply->str;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);

	std::cout << "Succeed to execute command [ GET " << key << "  ]" << std::endl;
	return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value) {
	//ִ��redis������

	auto connect = _redisPool->getConnection();

	auto reply = (redisReply*)redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());

	//�������NULL��˵��ִ��ʧ��
	if (NULL == reply)
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	//���ִ��ʧ�����ͷ�����
	if (!(reply->type == REDIS_REPLY_STATUS && (strcmp(reply->str, "OK") == 0 || strcmp(reply->str, "ok") == 0)))
	{
		std::cout << "Execut command [ SET " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	//ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	std::cout << "Execut command [ SET " << key << "  " << value << " ] success ! " << std::endl;
	return true;
}

bool RedisMgr::Auth(const std::string& password)
{
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "AUTH %s", password.c_str());
	if (reply->type == REDIS_REPLY_ERROR) {
		std::cout << "��֤ʧ��" << std::endl;
		//ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	else {
		//ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
		freeReplyObject(reply);
		std::cout << "��֤�ɹ�" << std::endl;
		_redisPool->returnConnection(connect);
		return true;
	}
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value) {

	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "LPOP %s ", key.c_str());
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ LPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value) {
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
	if (NULL == reply)
	{
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	if (reply->type != REDIS_REPLY_INTEGER || reply->integer <= 0) {
		std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}

	std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

bool RedisMgr::RPop(const std::string& key, std::string& value) {
	auto connect = _redisPool->getConnection();

	auto reply = (redisReply*)redisCommand(connect, "RPOP %s ", key.c_str());
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	value = reply->str;
	std::cout << "Execut command [ RPOP " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value) {
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}


bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
	const char* argv[4];
	size_t argvlen[4];
	argv[0] = "HSET";
	argvlen[0] = 4;
	argv[1] = key;
	argvlen[1] = strlen(key);
	argv[2] = hkey;
	argvlen[2] = strlen(hkey);
	argv[3] = hvalue;
	argvlen[3] = hvaluelen;

	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommandArgv(connect, 4, argv, argvlen);
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
	const char* argv[3];
	size_t argvlen[3];
	argv[0] = "HGET";
	argvlen[0] = 4;
	argv[1] = key.c_str();
	argvlen[1] = key.length();
	argv[2] = hkey.c_str();
	argvlen[2] = hkey.length();
	auto connect = _redisPool->getConnection();

	auto reply = (redisReply*)redisCommandArgv(connect, 3, argv, argvlen);
	if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
		freeReplyObject(reply);
		std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
		_redisPool->returnConnection(connect);
		return "";
	}

	std::string value = reply->str;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	std::cout << "Execut command [ HGet " << key << " " << hkey << " ] success ! " << std::endl;
	return value;
}

bool RedisMgr::Del(const std::string& key)
{
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "DEL %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER) {
		std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	std::cout << "Execut command [ Del " << key << " ] success ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
	auto connect = _redisPool->getConnection();
	auto reply = (redisReply*)redisCommand(connect, "exists %s", key.c_str());
	if (reply == nullptr || reply->type != REDIS_REPLY_INTEGER || reply->integer == 0) {
		std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
		freeReplyObject(reply);
		_redisPool->returnConnection(connect);
		return false;
	}
	std::cout << " Found [ Key " << key << " ] exists ! " << std::endl;
	freeReplyObject(reply);
	_redisPool->returnConnection(connect);
	return true;
}

void RedisMgr::Close()
{
	_redisPool->close();
}

RedisMgr::RedisMgr() {
	auto configMgr = ConfigMgr::Inst();
	auto host = configMgr["Redis"]["Host"];
	auto port = configMgr["Redis"]["Port"];
	auto pwd = configMgr["Redis"]["Passwd"];

	this->_redisPool.reset(new RedisPool(5, host.c_str(), atoi(port.c_str()), pwd.c_str()));
}

RedisMgr::~RedisMgr() {
	Close();
}
