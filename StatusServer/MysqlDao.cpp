#include "MysqlDao.h"
#include "ConfigMgr.h"

MysqlPool::MysqlPool(const std::string& url, const std::string& user, const std::string& pass, const std::string& schema, int poolSize):
	_url(url),_user(user),_pass(pass),_schema(schema),_poolSize(poolSize),_bstop(false)
{
	try
	{
		for (int i = 0; i < _poolSize; i++) {
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_driver_instance();
			auto* conn = driver->connect(_url, _user, _pass);
			conn->setSchema(_schema);
			//��ȡ��ǰ��ʱ���
			auto currentTime = std::chrono::system_clock::now().time_since_epoch();
			//��ʱ���ת��Ϊ��
			long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
			_pool.push(std::make_unique<SqlConnection>(conn, timestamp));

		}

		_check_thread = std::thread([this]() {
			
			while (!_bstop) {
				checkConnection();
				std::this_thread::sleep_for(std::chrono::seconds(60));
			}
			
		});
		_check_thread.detach();
	}
	catch (const std::exception& e)
	{
		// �����쳣
		std::cout << "mysql pool init failed, error is " << e.what() << std::endl;
	}
}


void MysqlPool::checkConnection() {
	/*
	std::lock_guard<std::mutex> guard(_mutex);
	int poolsize = _pool.size();

	//��ȡ��ǰʱ���
	auto currentTime = std::chrono::system_clock::now().time_since_epoch();
	//ʱ���תΪ��
	long long timestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime).count();
	
	for (int i = 0; i < poolsize; i++) {
		auto con = std::move(_pool.front());
		_pool.pop();

		Defer defer([this, &con]() {
			_pool.push(std::move(con));
		});

		if (timestamp - con->_last_oper_time < 5) {
			continue;
		}
		try {
			std::unique_ptr<sql::Statement> stmt(con->_con->createStatement());
			stmt->executeQuery("SELECT 1");
			con->_last_oper_time = timestamp;
			//std::cout << "execute timer alive query , cur is " << timestamp << std::endl;
		}
		catch (sql::SQLException& e) {
			std::cout << "Error keeping connection alive: " << e.what() << std::endl;
			// ���´������Ӳ��滻�ɵ�����
			sql::mysql::MySQL_Driver* driver = sql::mysql::get_mysql_driver_instance();
			auto* newcon = driver->connect(_url, _user, _pass);
			newcon->setSchema(_schema);
			con->_con.reset(newcon);
			con->_last_oper_time = timestamp;
		}

	}
	*/
	
}

std::unique_ptr<SqlConnection> MysqlPool::getConnection() {
	std::unique_lock<std::mutex> ul(_mutex);
	_cond.wait(ul, [this]() {
		
		return _bstop || !_pool.empty();
	});

	if (_bstop) {
		return nullptr;
	}
	auto con = std::move(_pool.front());
	_pool.pop();
	return con;
}

void  MysqlPool :: returnConnection(std::unique_ptr<SqlConnection> con) {
	std::unique_lock<std::mutex> ul(_mutex);
	if (_bstop) {
		return;
	}
	_pool.push(std::move(con));
	_cond.notify_one();
}

void MysqlPool::Close() {
	_bstop = true;
	_cond.notify_all();
}

MysqlPool::~MysqlPool() {
	std::unique_lock<std::mutex> lock(_mutex);
	while (!_pool.empty()) {
		_pool.pop();
	}
}

MysqlDao::MysqlDao(){
	auto& cfg = ConfigMgr::Inst();
	const auto& host = cfg["Mysql"]["Host"];
	const auto& port = cfg["Mysql"]["Port"];
	const auto& pwd = cfg["Mysql"]["Passwd"];
	const auto& schema = cfg["Mysql"]["Schema"];
	const auto& user = cfg["Mysql"]["User"];
	_pool.reset(new MysqlPool(host + ":" + port, user, pwd, schema, 5));
}

MysqlDao::~MysqlDao() {
	_pool->Close();
}

int MysqlDao::RegUser(const std::string& name, const std::string& email, const std::string& pwd) {
	auto con = _pool->getConnection();
	if (!con) return -1;
	try
	{
		//׼�����ô洢����
		/*
		prepareStatement��������һ��SQL�����Ϊ������������һ��Ԥ�����SQL������
		�洢���̿��Խ��ܶ�������������ܷ���һ�������һ�����ݿ�������������������@result��,Ҫ����myqsl�д�������洢����
		
		��ô���� ����Ż�

		CREATE PROCEDURE `reg_user`(IN `new_name` VARCHAR(255),
		IN `new_email` VARCHAR(255),
		IN `new_pwd` VARCHAR(255),
		OUT `result` INT)

		*/

		std::unique_ptr<sql::PreparedStatement> stmt(con->_con->prepareStatement("CALL reg_user(?,?,?,@result)"));
		stmt->setString(1, name);
		stmt->setString(2, email);
		stmt->setString(3, pwd);

		// ����PreparedStatement��ֱ��֧��ע�����������������Ҫʹ�ûỰ������������������ȡ���������ֵ

		// ִ�д洢����
		stmt->execute();
		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����
		// ���磬����洢����������һ���Ự����@result���洢������������������ȡ��

		std::unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @result AS result"));
		if (res->next()) {
			int result = res->getInt("result");
			std::cout << "Result: " << result << std::endl;
			_pool->returnConnection(std::move(con));
			return result;
		}
		_pool->returnConnection(std::move(con));
		return -1;
	}
	catch (sql::SQLException& e)
	{
		_pool->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << "�� MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;
	}
}

int MysqlDao::RegUserTransaction(const std::string& name, const std::string& email, const std::string& pwd,
	const std::string& icon) {

	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}
	try
	{
		//��ʼ����
		con->_con->setAutoCommit(false);

		//ִ�е�һ�����ݿ����������email�����û�
		std::unique_ptr<sql::PreparedStatement> pstmt_email(con->_con->prepareStatement("SELECT 1 FROM user WHERE email = ?"));
		// �󶨲���
		pstmt_email->setString(1, email);
		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res_email(pstmt_email->executeQuery());
		auto email_exist = res_email->next();

		if (email_exist) {
			con->_con->rollback();
			std::cout << "email " << email << " exist";
			return 0;
		}

		// ׼����ѯ�û����Ƿ��ظ�
		std::unique_ptr<sql::PreparedStatement> pstmt_name(con->_con->prepareStatement("SELECT 1 FROM user WHERE name = ?"));
		// �󶨲���
		pstmt_name->setString(1, name);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res_name(pstmt_name->executeQuery());

		auto name_exist = res_name->next();
		if (name_exist) {
			con->_con->rollback();
			std::cout << "name " << name << " exist";
			return 0;
		}

		// ׼������user_id���id
		std::unique_ptr<sql::PreparedStatement> pstmt_upid(con->_con->prepareStatement("UPDATE user_id SET id = id + 1"));
		// ִ�и���
		pstmt_upid->executeUpdate();
		std::unique_ptr<sql::PreparedStatement> pstmt_uid(con->_con->prepareStatement("SELECT id FROM user_id"));
		std::unique_ptr<sql::ResultSet> res_uid(pstmt_uid->executeQuery());
		int newId = 0;
		// ��������
		if (res_uid->next()) {
			newId = res_uid->getInt("id");
		}
		else {
			std::cout << "select id from user_id failed" << std::endl;
			con->_con->rollback();
			return -1;
		}

		// ����user��Ϣ
		std::unique_ptr<sql::PreparedStatement> pstmt_insert(con->_con->prepareStatement("INSERT INTO user (uid, name, email, pwd, nick, icon) VALUES (?,?,?,?,?)"));
		pstmt_insert->setInt(1, newId);
		pstmt_insert->setString(2, name);
		pstmt_insert->setString(3, email);
		pstmt_insert->setString(4, pwd);
		pstmt_insert->setString(5, name);
		pstmt_insert->setString(6, icon);
		//ִ�в���
		pstmt_insert->executeUpdate();
		// �ύ����
		con->_con->commit();
		std::cout << "newuser insert into user success" << std::endl;
		return newId;

	}
	catch (sql::SQLException& e)
	{
		if (con) {
			con->_con->rollback();
		}
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return -1;

	}
}

bool MysqlDao::CheckEmail(const std::string& name, const std::string& email) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT email FROM user WHERE name = ?"));

		// �󶨲���
		pstmt->setString(1, name);

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

		// ���������
		while (res->next()) {
			std::cout << "Check Email: " << res->getString("email") << std::endl;
			if (email != res->getString("email")) {
				_pool->returnConnection(std::move(con));
				return false;
			}
			_pool->returnConnection(std::move(con));
			return true;
		}
		return false;
	}
	catch (sql::SQLException& e) {
		_pool->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::UpdatePwd(const std::string& name, const std::string& newpwd) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		// ׼����ѯ���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("UPDATE user SET pwd = ? WHERE name = ?"));

		// �󶨲���
		pstmt->setString(2, name);
		pstmt->setString(1, newpwd);

		// ִ�и���
		int updateCount = pstmt->executeUpdate();

		std::cout << "Updated rows: " << updateCount << std::endl;
		_pool->returnConnection(std::move(con));
		return true;
	}
	catch (sql::SQLException& e) {
		_pool->returnConnection(std::move(con));
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	auto con = _pool->getConnection();
	if (con == nullptr) {
		return false;
	}

	Defer defer([this, &con]() {
		_pool->returnConnection(std::move(con));
		});

	try {


		// ׼��SQL���
		std::unique_ptr<sql::PreparedStatement> pstmt(con->_con->prepareStatement("SELECT * FROM user WHERE name = ?"));
		pstmt->setString(1, email); // ��username�滻Ϊ��Ҫ��ѯ���û���

		// ִ�в�ѯ
		std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
		std::string origin_pwd = "";
		// ���������
		while (res->next()) {
			origin_pwd = res->getString("pwd");
			// �����ѯ��������
			std::cout << "Password: " << origin_pwd << std::endl;
			break;
		}

		if (pwd != origin_pwd) {
			return false;
		}
		userInfo.name = res->getString("name");
		userInfo.email = res->getString("email");
		userInfo.uid = res->getInt("uid");
		userInfo.pwd = origin_pwd;
		return true;
	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}

bool MysqlDao::TestProcedure(const std::string& email, int& uid, std::string& name) {
	auto con = _pool->getConnection();
	try {
		if (con == nullptr) {
			return false;
		}

		Defer defer([this, &con]() {
			_pool->returnConnection(std::move(con));
			});
		// ׼�����ô洢����
		std::unique_ptr < sql::PreparedStatement > stmt(con->_con->prepareStatement("CALL test_procedure(?,@userId,@userName)"));
		// �����������
		stmt->setString(1, email);

		// ����PreparedStatement��ֱ��֧��ע�����������������Ҫʹ�ûỰ������������������ȡ���������ֵ

		  // ִ�д洢����
		stmt->execute();
		// ����洢���������˻Ự��������������ʽ��ȡ���������ֵ�������������ִ��SELECT��ѯ����ȡ����
	   // ���磬����洢����������һ���Ự����@result���洢������������������ȡ��
		std:: unique_ptr<sql::Statement> stmtResult(con->_con->createStatement());
		std::unique_ptr<sql::ResultSet> res(stmtResult->executeQuery("SELECT @userId AS uid"));
		if (!(res->next())) {
			return false;
		}

		uid = res->getInt("uid");
		std::cout << "uid: " << uid << std::endl;

		stmtResult.reset(con->_con->createStatement());
		res.reset(stmtResult->executeQuery("SELECT @userName AS name"));
		if (!(res->next())) {
			return false;
		}

		name = res->getString("name");
		std::cout << "name: " << name << std:: endl;
		return true;

	}
	catch (sql::SQLException& e) {
		std::cerr << "SQLException: " << e.what();
		std::cerr << " (MySQL error code: " << e.getErrorCode();
		std::cerr << ", SQLState: " << e.getSQLState() << " )" << std::endl;
		return false;
	}
}


