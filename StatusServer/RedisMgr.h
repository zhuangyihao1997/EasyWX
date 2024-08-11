#pragma once
#include "Singleton.h"
#include <hiredis.h>
#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>

class RedisPool 
{
public:
	RedisPool(std::size_t poolSize, const char* host, int port, const char* pwd);
	~RedisPool();
	redisContext* getConnection();
	void returnConnection(redisContext* context);
	void close();

private:
	std::size_t _poolSize;
	const char* _host;
	const char* _pwd;
	int _port;

	std::queue<redisContext*> connections_;
	std::mutex _mutex;
	std::condition_variable _cv;
	std::atomic_bool b_stop;

};

class RedisMgr : public Singleton<RedisMgr>,public std::enable_shared_from_this<RedisMgr>
{
	friend class Singleton<RedisMgr>;
public:
	~RedisMgr();
	bool Connect(const std::string& host, int port);
	bool Get(const std::string& key, std::string& value);
	bool Set(const std::string& key, const std::string& value);
	bool Auth(const std::string& password);
	bool LPush(const std::string& key, const std::string& value);
	bool LPop(const std::string& key, std::string& value);
	bool RPush(const std::string& key, const std::string& value);
	bool RPop(const std::string& key, std::string& value);
	bool HSet(const std::string& key, const std::string& hkey, const std::string& value);
	bool HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen);
	std::string HGet(const std::string& key, const std::string& hkey);
	bool Del(const std::string& key);
	bool ExistsKey(const std::string& key);
	void Close();

private:
	RedisMgr();
	std::unique_ptr<RedisPool> _redisPool;
};

