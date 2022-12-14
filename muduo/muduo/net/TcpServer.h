// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "muduo/base/Atomic.h"
#include "muduo/base/Types.h"
#include "muduo/net/TcpConnection.h"

#include <map>

namespace muduo
{
namespace net
{

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer : noncopyable
{
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;
  enum Option
  {
    kNoReusePort,
    kReusePort,
  };

  //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  TcpServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg,
            Option option = kNoReusePort);
  ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

  const string& ipPort() const { return ipPort_; }
  const string& name() const { return name_; }
  EventLoop* getLoop() const { return loop_; }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  // 设置poll的thread的数量（一般默认是单reactor）
  void setThreadNum(int numThreads);
  // 设置thread的callback
  void setThreadInitCallback(const ThreadInitCallback& cb)
  { threadInitCallback_ = cb; }
  /// valid after calling start()
  std::shared_ptr<EventLoopThreadPool> threadPool()
  { return threadPool_; }

  /// Starts the server if it's not listening.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  // 将acceptor中的listen函数放入runInLoop中执行
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  // 出现新的connection（acceptor的fd发生了可读事件）
  void newConnection(int sockfd, const InetAddress& peerAddr);
  /// Thread safe.
  // 将当前的connection移除（注册remove connection callback到loop中）
  void removeConnection(const TcpConnectionPtr& conn);
  /// Not thread safe, but in loop
  // 实际上移除connection的callback
  void removeConnectionInLoop(const TcpConnectionPtr& conn);

  typedef std::map<string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;  // the acceptor loop
  // 端口号
  const string ipPort_;
  // server的name
  const string name_;
  // 指向acceptor的指针
  std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  // 用于实现多recator模式的线程池
  std::shared_ptr<EventLoopThreadPool> threadPool_;
  // 创建connection时的ptr
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  // write complete的callback
  WriteCompleteCallback writeCompleteCallback_;
  // thread的callback
  ThreadInitCallback threadInitCallback_;
  // 表示tcpserver是否开始start
  AtomicInt32 started_;
  // 记录fd的数量（acceptor中的fd也包括）
  int nextConnId_;
  // 存放当前server所有的连接（key为每个连接的姓名，value为connection的指针）
  ConnectionMap connections_;
};

}  // namespace net
}  // namespace muduo

#endif  // MUDUO_NET_TCPSERVER_H
