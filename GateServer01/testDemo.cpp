#include "testDemo.h"

void TestRedis(){
    
     //����redis ��Ҫ�����ſ��Խ�������
    //redisĬ�ϼ����˿�Ϊ6387 �����������ļ����޸�
    redisContext* c = redisConnect("127.0.0.1", 6380);
    if (c->err)
    {
        printf("Connect to redisServer faile:%s\n", c->errstr);
        redisFree(c);        return;
    }
    printf("Connect to redisServer Success\n");

    
    std::string redis_password = "123456";

    const char* command = "AUTH %s";
    redisReply* r = (redisReply*)redisCommand(c, "AUTH 123456");

    if (r->type == REDIS_REPLY_ERROR) {
        printf("Redis��֤ʧ�ܣ�\n");
    }
    else {
        printf("Redis��֤�ɹ���\n");
    }

    //Ϊredis����key
    const char* command1 = "set stest1 value1";

    //ִ��redis������
    r = (redisReply*)redisCommand(c, command1);

    //�������NULL��˵��ִ��ʧ��
    if (NULL == r)
    {
        printf("Execut command1 failure\n");
        redisFree(c);        return;
    }

    //���ִ��ʧ�����ͷ�����
    if (!(r->type == REDIS_REPLY_STATUS && (strcmp(r->str, "OK") == 0 || strcmp(r->str, "ok") == 0)))
    {
        printf("Failed to execute command[%s]\n", command1);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    //ִ�гɹ� �ͷ�redisCommandִ�к󷵻ص�redisReply��ռ�õ��ڴ�
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command1);

    const char* command2 = "strlen stest1";
    r = (redisReply*)redisCommand(c, command2);

    //����������Ͳ������� ���ͷ�����
    if (r->type != REDIS_REPLY_INTEGER)
    {
        printf("Failed to execute command[%s]\n", command2);
        freeReplyObject(r);
        redisFree(c);        return;
    }

    std::string redis_password2 = "123456";
    redisReply* r1 = (redisReply*)redisCommand(c, "AUTH %s", redis_password2.c_str());
    if (r1->type == REDIS_REPLY_ERROR) {
        printf("Redis��֤ʧ�ܣ�\n");
    }
    else {
        printf("Redis��֤�ɹ���\n");
    }
    

    //��ȡ�ַ�������
    int length = r->integer;
    freeReplyObject(r);
    printf("The length of 'stest1' is %d.\n", length);
    printf("Succeed to execute command[%s]\n", command2);

    //��ȡredis��ֵ����Ϣ
    const char* command3 = "get stest1";
    r = (redisReply*)redisCommand(c, command3);
    if (r->type != REDIS_REPLY_STRING)
    {
        printf("Failed to execute command[%s]\n", command3);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    printf("The value of 'stest1' is %s\n", r->str);
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command3);

    const char* command4 = "get stest2";
    r = (redisReply*)redisCommand(c, command4);
    if (r->type != REDIS_REPLY_NIL)
    {
        printf("Failed to execute command[%s]\n", command4);
        freeReplyObject(r);
        redisFree(c);        return;
    }
    freeReplyObject(r);
    printf("Succeed to execute command[%s]\n", command4);

    //�ͷ�������Դ
    redisFree(c);

   
}
