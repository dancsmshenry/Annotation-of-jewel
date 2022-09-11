// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//

#include "muduo/net/Buffer.h"

#include "muduo/net/SocketsOps.h"

#include <errno.h>
#include <sys/uio.h>

using namespace muduo;
using namespace muduo::net;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::readFd(int fd, int* savedErrno)
{ // 函数的巧妙之处在于，它能够一次性把所有的数据都读到缓冲区中
  // saved an ioctl()/FIONREAD call to tell how much to read
  // 这里在栈上开辟了64kb的空间
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  // 第一块缓存空间，加上writeindex是因为前面的位置是有数据的
  vec[0].iov_base = begin()+writerIndex_;
  vec[0].iov_len = writable;
  // 第二块缓存空间
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  // 调用readv函数读取数据（返回的是读到的总字节数；如果报错就返回-1）
  const ssize_t n = sockets::readv(fd, vec, iovcnt);
  if (n < 0)
  {// 读数据发生错误
    *savedErrno = errno;
  }
  else if (implicit_cast<size_t>(n) <= writable)
  {// 读到的数据都放在第一个缓冲区buffer中
    writerIndex_ += n;
  }
  else
  {// 读到的数据需要额外的空间（即栈上的空间）存放，即要把栈上的数据放到buffer中
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf)
  // {
  //   goto line_30;
  // }
  return n;
}

