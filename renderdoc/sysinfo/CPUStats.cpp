/******************************************************************************

The MIT License (MIT)


Copyright (C) 2021 OPPO LLC


Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:


The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
******************************************************************************/

#include "sysinfo/CPUStats.h"
#include "common/common.h"
#include <assert.h>
#include <stdarg.h>
#include <atomic>

class CPUStats::Impl
{
public:
  Impl() {}
  ~Impl()
  {
    if(_t.joinable())
    {
      _running = false;
      _t.join();
    }
  }
  void start()
  {
    if(++_ref == 1)
    {
      RDCDEBUG("DEBUG_CPU : start CPU stats thread.");
      _running = true;
      _t = std::thread([this] { threadProc(); });
    }
  }
  void stop()
  {
    if(--_ref == 0 && _t.joinable())
    {
      RDCDEBUG("DEBUG_CPU : stop CPU stats thread.");
      _running = false;
      _t.join();
    }
  }

  std::thread _t;
  std::atomic<bool> _running;
  int _ref = 0;
  int _captureNum = 0;

private:
  enum CPUStates
  {
    USER = 0,
    NICE,
    SYSTEM,
    IDLE,
    IOWAIT,
    IRQ,
    SOFTIRQ,
    STEAL,
    GUEST,
    GUEST_NICE
  };
  enum appCPUStates
  {
    UTIME = 0,
    STIME,
    CUTIME,
    CSTIME,
    TASKCPU
  };

  static const int NUM_CPU_STATES = 10;
  typedef struct CPUData
  {
    std::string cpu;
    size_t times[NUM_CPU_STATES];
  } CPUData;

  static const int NUM_APP_CPU_STATES = 5;
  typedef struct appCPUData
  {
    std::string cpu;
    size_t times[NUM_APP_CPU_STATES];
  } appCPUData;

  std::vector<CPUData> _entries1;
  std::vector<CPUData> _entries2;
  appCPUData _entry1;
  appCPUData _entry2;
  std::ofstream _history;
  int appCoreNum = 0;

private:
  static bool recreateStatsFile(std::ofstream &f, const char *path)
  {
    if(access(path, F_OK) != -1)
    {
      return true;
    }
    f.open(path);
    if(f.good())
    {
      auto header =
          "frameNumber, "
          "app_core_number, cpu_freq_0, cpu_freq_4, cpu_freq_6, cpu_freq_7, cpu_App_on, "
          "cpu_total_active, cpu_total_idle, "
          "cpu_0_active(%), cpu_0_idle(%), "
          "cpu_1_active(%), cpu_1_idle(%), "
          "cpu_2_active(%), cpu_2_idle(%), "
          "cpu_3_active(%), cpu_3_idle(%), "
          "cpu_4_active(%), cpu_4_idle(%), "
          "cpu_5_active(%), cpu_5_idle(%), "
          "cpu_6_active(%), cpu_6_idle(%), "
          "cpu_7_active(%), cpu_7_idle(%), "
          "appCPUUsage(%), appSingleCPUUsage(%) ";
      f << header << std::endl;
      return true;
    }
    else
    {
      // this failure is expected when the cpu stats folder is not there.
      return false;
    }
  }

  static void recreateHistoryFile(std::ofstream &f)
  {
    const char *path = "/data/local/tmp/jedi/cpustats.csv";
    if(recreateStatsFile(f, path))
    {
      RDCLOG("CPU Stat history file %s created.", path);
    }
    else
    {
      RDCERR("Failed to create CPU stats file %s", path);
    }
  }

  void threadProc()
  {    
    readStatsCPU(_entries1);
    readStatsAppCPU(_entry1);
    appCoreNum = static_cast<int>(_entry1.times[TASKCPU]);

    // stores stats history
    recreateHistoryFile(_history);

    while(_running)
    {
      // keep cpu stats thread running.
    }

    // start reading data.
    readStatsCPU(_entries2);
    readStatsAppCPU(_entry2);
    saveStatsApp(_entries1, _entries2, _entry1, _entry2);

    _entries1.clear();
    _entries2.clear();
  }

  static std::string readSysFile(const std::string &file)
  {
    std::string val;
    std::ifstream ifile(file);
    if(ifile.good())
    {
      ifile >> val;
    }
    else
    {
      val = "<err>";
    }
    return val;
  }

  static const size_t FORMAT_BUFFER_SIZE = 4 * 1024;
  char *formatString(const char *format, ...)
  {
    va_list args;
    va_start(args, format);
    thread_local static std::vector<char> buf1(FORMAT_BUFFER_SIZE);
    printToVector(std::vsnprintf, buf1, format, args);
    va_end(args);
    return buf1.data();
  }

  template <class PRINTF, class... ARGS>
  void printToVector(PRINTF p, std::vector<char> &buffer, ARGS... args)
  {
    int nn = p(buffer.data(), buffer.size(), args...);
    if(nn <= 0)
    {
      return;
    }
    if((size_t)nn > buffer.size())
    {
      buffer.resize((size_t)nn + 1);
    }
    nn = p(buffer.data(), buffer.size(), args...);
    assert((size_t)nn <= buffer.size());
  }

  std::string readCpuFreq(int core)
  {
    return readSysFile(formatString("/sys/devices/system/cpu/cpu%d/cpufreq/cpuinfo_cur_freq", core));
  }

  void readStatsCPU(std::vector<CPUData> &entries)
  {
    std::ifstream fileStat("/proc/stat");
    std::string line;

    const std::string strCPU("cpu");
    const std::size_t lenStrCPU = strCPU.size();
    const std::string strTotal("total");

    while(std::getline(fileStat, line))
    {
      if(!line.compare(0, lenStrCPU, strCPU))
      {
        std::istringstream ss(line);

        entries.emplace_back(CPUData());
        CPUData &entry = entries.back();

        ss >> entry.cpu;

        if(entry.cpu.size() > lenStrCPU)
          entry.cpu.erase(0, lenStrCPU);
        else
          entry.cpu = strTotal;

        for(int i = 0; i < NUM_CPU_STATES; ++i)
          ss >> entry.times[i];
      }
    }
  }

  size_t getIdleTime(const CPUData &e) { return e.times[IDLE] + e.times[IOWAIT]; }

  size_t getActiveTime(const CPUData &e)
  {
    return e.times[USER] + e.times[NICE] + e.times[SYSTEM] + e.times[IRQ] + e.times[SOFTIRQ] +
           e.times[STEAL] + e.times[GUEST] + e.times[GUEST_NICE];
  }

  void saveStats(const std::vector<CPUData> &entries1, const std::vector<CPUData> &entries2)
  {
    for(size_t i = 0; i < entries1.size(); ++i)
    {
      const CPUData &e1 = entries1[i];
      const CPUData &e2 = entries2[i];

      const float activeTime = static_cast<float>(getActiveTime(e2) - getActiveTime(e1));
      const float idleTime = static_cast<float>(getIdleTime(e2) - getIdleTime(e1));
      const float totalTime = activeTime + idleTime;

      float activePercent = 100.f * activeTime / totalTime;
      float idlePercent = 100.f * idleTime / totalTime;
    }
  }

  void readStatsAppCPU(appCPUData &entry)
  {
    // get app/game pid
    int pid = getpid();

    std::ifstream fileStat("/proc/" + std::to_string(pid) + "/stat");

    std::string line = "";
    if(!std::getline(fileStat, line))
    {
      RDCERR("No /proc/%d/stat is found!", pid);
      return;
    }

    std::string delimiter = " ";
    size_t start = 0;
    auto end = line.find(delimiter);
    int count = 0;
    int i = 0;
    while(end != std::string::npos)
    {
      if((count > 12 && count < 17) || count == 38)
      {
        // read times
        entry.cpu = line.substr(start, end - start);
        std::istringstream ss(entry.cpu);
        ss >> entry.times[i++];
      }
      start = end + delimiter.length();
      end = line.find(delimiter, start);
      count++;
    }
  }

  size_t getAppCPUTime(const appCPUData &e)
  {
    return e.times[UTIME] + e.times[STIME] + e.times[CUTIME] + e.times[CSTIME];
  }

  void saveStatsApp(const std::vector<CPUData> &entries1, const std::vector<CPUData> &entries2,
                    const appCPUData &entry1, const appCPUData &entry2)
  {
    const size_t NUM_ENTRIES = entries1.size();
    float appCPUTotaltime = 0.0f;
    float appSingleCPUTotaltime = 0.0f;

    float activePercents[NUM_ENTRIES];
    float idlePercents[NUM_ENTRIES];
    for(size_t i = 0; i < NUM_ENTRIES; ++i)
    {
      const CPUData &e1 = entries1[i];
      const CPUData &e2 = entries2[i];

      const float activeTime = static_cast<float>(getActiveTime(e2) - getActiveTime(e1));
      const float idleTime = static_cast<float>(getIdleTime(e2) - getIdleTime(e1));
      const float totalTime = activeTime + idleTime;

      activePercents[i] = 100.f * activeTime / totalTime;
      idlePercents[i] = 100.f * idleTime / totalTime;

      if(i == 0)
      {
        appCPUTotaltime = totalTime;
      }

      if(i == entry1.times[TASKCPU])
      {
        appSingleCPUTotaltime = totalTime;
      }
    }

    const float appCPUTime = static_cast<float>(getAppCPUTime(entry2) - getAppCPUTime(entry1));
    float appCPUPercent = 100.f * (appCPUTime / appCPUTotaltime);

    // Single CPU usage matches the value in "adb top -p PID"
    float appSingleCPUPercent = 100.f * (appCPUTime / appSingleCPUTotaltime);

    std::stringstream values;
    values << _captureNum << ", " << appCoreNum << ", " << readCpuFreq(0) << ", " << readCpuFreq(4)
           << ", " << readCpuFreq(6) << ", " << readCpuFreq(7) << ", " << readCpuFreq(appCoreNum)
           << ", " << activePercents[0] << ", " << idlePercents[0] << ", " << activePercents[1]
           << ", " << idlePercents[1] << ", " << activePercents[2] << ", " << idlePercents[2]
           << ", " << activePercents[3] << ", " << idlePercents[3] << ", " << activePercents[4]
           << ", " << idlePercents[4] << ", " << activePercents[5] << ", " << idlePercents[5]
           << ", " << activePercents[6] << ", " << idlePercents[6] << ", " << activePercents[7]
           << ", " << idlePercents[7] << ", " << activePercents[8] << ", " << idlePercents[8]
           << ", " << appCPUPercent << ", " << appSingleCPUPercent;
    _history << values.str() << std::endl;
  }
};

CPUStats::CPUStats() : _impl(new Impl()) {}

CPUStats::~CPUStats()
{
  delete _impl;
}

void CPUStats::onStartCapture()
{
  _impl->start();
};

void CPUStats::onEndCapture()
{
  _impl->stop();
};

void CPUStats::setCaptureNum(uint32_t captureNum)
{
  _impl->_captureNum = captureNum;
}
