// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/LogFile.h"

#include "muduo/base/FileUtil.h"
#include "muduo/base/ProcessInfo.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

using namespace muduo;

LogFile::LogFile(const string& basename,
                 off_t rollSize,
                 bool threadSafe,
                 int flushInterval,
                 int checkEveryN)
  : basename_(basename),
    rollSize_(rollSize), // 一个文件的最大字节数
    flushInterval_(flushInterval),  //  允许刷新的最大间隙
    checkEveryN_(checkEveryN),  //  允许停留在buffer的最大日志数
    count_(0),  //  当前写入的条数，最大为checkEveryN
    mutex_(threadSafe ? new MutexLock : NULL),
    startOfPeriod_(0),  //  记录前一天的时间，以秒记录
    lastRoll_(0), //  上一批rollfile的时间，以秒记录
    lastFlush_(0) //  上一次刷新的时间，以秒记录
{
  assert(basename.find('/') == string::npos); //  判断文件名是否合法
  rollFile();
}

LogFile::~LogFile() = default;

void LogFile::append(const char* logline, int len)
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  }
  else
  {
    append_unlocked(logline, len);
  }
}

void LogFile::flush()
{
  if (mutex_)
  {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  }
  else
  {
    file_->flush();
  }
}

void LogFile::append_unlocked(const char* logline, int len)
{
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_)
  {
    rollFile();
  }
  else
  {
    ++count_;
    if (count_ >= checkEveryN_)
    {
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_)
      {
        rollFile();
      }
      else if (now - lastFlush_ > flushInterval_)
      {
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

bool LogFile::rollFile()
{
  time_t now = 0;
  //  now得到的是当前的unix时间戳（是自1970年1月1日00:00:00 GMT以来的秒数）
  string filename = getLogFileName(basename_, &now);
  // 计算当前是当天过了多少秒
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  // 如果当前的天数大于上次刷盘的时间
  if (now > lastRoll_)
  {
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start; //  记录上一次rollfile的日期（天）
    // 换一个文件写日志（为保证两天的日志不写在同一个文件中 而上一天的日志可能并未写到rollSize_大小）
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}

string LogFile::getLogFileName(const string& basename, time_t* now)
{
  string filename;
  // 为什么是64：日期在格式化为字符串后为64
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm); // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  filename += ProcessInfo::hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}

