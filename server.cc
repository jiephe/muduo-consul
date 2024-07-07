#include "muduo/net/TcpServer.h"

#include "muduo/base/Atomic.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Thread.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/InetAddress.h"

#include "muduo/net/http/HttpServer.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include <string>
#include <utility>

#include <stdio.h>
#include <unistd.h>

#include "ppconsul/agent.h"
using ppconsul::Consul;
using namespace ppconsul::agent;

using namespace muduo;
using namespace muduo::net;


void onRequest(const HttpRequest& req, HttpResponse* resp)
{
  if (req.path() == "/health")
  {
      // Respond with a 200 OK status code if the service is healthy
      resp->setStatusCode(HttpResponse::k200Ok);
      resp->setStatusMessage("OK");
      resp->setCloseConnection(true); // Close the connection after sending the response
  }
  else
  {
      // Respond with a 404 Not Found if the request path is not recognized
      resp->setStatusCode(HttpResponse::k404NotFound);
      resp->setStatusMessage("Not Found");
      resp->setCloseConnection(true);
  }
}

void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    conn->setTcpNoDelay(true);
  }
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp)
{
  LOG_INFO << "onMessage pid = " << getpid() << ", tid = " << CurrentThread::tid() << " msg: " << buf->retrieveAllAsString();
  //conn->send(buf);
}

void regist(const char* ip, uint16_t tcp_port, uint16_t http_port)
{
  // Create a consul client that uses default local endpoint `http://127.0.0.1:8500` and default data center
  Consul consul("http://172.16.13.71:8500");
  // We need the 'agent' endpoint for a service registration
  Agent agent(consul);

  std::string httpS("http://");
  httpS += std::string(ip) + std::string(":");
  httpS += std::to_string(http_port);
  httpS.append("/health");

  std::string serviceID = "my-service-";
  serviceID.append(std::to_string(tcp_port));

  // Register a service with associated HTTP check:
  agent.registerService(
      kw::name = "my-service",
      kw::id = serviceID,  // 唯一的服务 ID
      kw::address = ip, // 指定IP地址
      kw::port = tcp_port,
      kw::tags = {"tcp", "pong_server"},
      kw::check = HttpCheck{httpS, std::chrono::seconds(2)}
  );
}

int main(int argc, char* argv[])
{
  if (argc < 4)
  {
    fprintf(stderr, "Usage: server <address> <port> <threads>\n");
  }
  else
  {
    LOG_INFO << "pid1 = " << getpid() << ", tid = " << CurrentThread::tid();
    Logger::setLogLevel(Logger::INFO);

    const char* ip = argv[1];
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress listenAddr(ip, port);
    int threadCount = atoi(argv[3]);

    EventLoop loop;

    TcpServer server(&loop, listenAddr, "PingPong");

    server.setConnectionCallback(onConnection);
    server.setMessageCallback(onMessage);

    if (threadCount > 1)
    {
      server.setThreadNum(threadCount);
    }

    server.start();

    //http check server
    uint16_t http_port = static_cast<uint16_t>(atoi(argv[4]));
    HttpServer httpserver(&loop, InetAddress(http_port), "HealthCheckHttpServer");
    httpserver.setHttpCallback(onRequest);
    httpserver.setThreadNum(1);
    httpserver.start();

    regist(ip, port, http_port);

    loop.loop();
  }
}

