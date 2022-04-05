#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <chrono>
#include <math.h>
#include <thread>

#include <CppLinuxSerial/SerialPort.hpp>

using namespace std;
using namespace std::chrono;
using namespace mn::CppLinuxSerial;

SerialPort* rfSource;

#define ADC_SAMPLE_RATE 122.880e6
#define BYTES_PER_SAMPLE 2
#define CHANNEL_COUNT 2
#define CMA_ALLOC _IOWR('Z', 0, uint32_t)

int __aeabi_idiv0(int return_value) {
  return 0;
}

long long __aeabi_ldiv0(long long return_value) {
  return 0;
}

void setFrequency(int frequency, int intermediateFrequency) {
  rfSource->Write("C0");
  rfSource->Write("f" + std::to_string(frequency));
  rfSource->Write("C1\n");
  rfSource->Write("f" + std::to_string(frequency + intermediateFrequency));
}

// 4096 bytes per page

int main() {
  int startFrequency   = 1000;
  int stepFrequency    = 20;
  int frequencyCount   = 101;
  int intermediateFreq = 32;
  int transmitPower    = 0;
  int loPower          = 15;
  uint32_t sampleCount = 1024;

  long long int sampleTimeInNs = (1 / ADC_SAMPLE_RATE) * sampleCount * 1000000000;

  //uint32_t settlingSamples = ADC_SAMPLE_RATE * 0.001;
  
  rfSource = new SerialPort("/dev/ttyACM0", BaudRate::B_57600, NumDataBits::EIGHT, Parity::NONE, NumStopBits::ONE);
  rfSource->SetTimeout(0);
  rfSource->Open();

  rfSource->Write("C0");
  rfSource->Write("W" + std::to_string(transmitPower));
  rfSource->Write("C1");
  rfSource->Write("W" + std::to_string(loPower));

  setFrequency(startFrequency, intermediateFreq);

  rfSource->Write("C0");

  rfSource->Write("E1");
  rfSource->Write("r1");

  rfSource->Write("C1");

  rfSource->Write("E1");
  rfSource->Write("r1");

  int fd, i;
  volatile uint8_t *rst;
  volatile void *cfg;
  volatile int16_t *ram;
  uint32_t size;
  int16_t value[2];

  if((fd = open("/dev/mem", O_RDWR)) < 0) {
    perror("open");
    return EXIT_FAILURE;
  }

  cfg = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x40000000);

  close(fd);

  if((fd = open("/dev/cma", O_RDWR)) < 0) {
    perror("open");
    return EXIT_FAILURE;
  }

  // rounded up to nearest page
  size = ceil( (sampleCount * BYTES_PER_SAMPLE * frequencyCount * CHANNEL_COUNT) / sysconf(_SC_PAGESIZE) ) * sysconf(_SC_PAGESIZE);

  if(ioctl(fd, CMA_ALLOC, &size) < 0) {
    perror("ioctl");
    return EXIT_FAILURE;
  }

  ram = static_cast<volatile int16_t*>(mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0));

  rst = (uint8_t *)(cfg + 0);

  // set number of samples
  *(uint32_t *)(cfg + 8) = sampleCount;

  int64_t startTime = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  for(int i = 0; i < frequencyCount; i++) {
    // set writer address
    *(uint32_t *)(cfg + 4) = sampleCount * BYTES_PER_SAMPLE * CHANNEL_COUNT * i;

    // reset writer
    *rst &= ~4;
    *rst |= 4;

    // reset packetizer
    *rst &= ~2;
    *rst |= 2;

    // wait for sample
    std::this_thread::sleep_for(std::chrono::nanoseconds(sampleTimeInNs));

    setFrequency(startFrequency + (i * stepFrequency), intermediateFreq);

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  int64_t endTime = duration_cast<microseconds>(system_clock::now().time_since_epoch()).count();

  printf("Sweep Done, took %lld microseconds\n", endTime - startTime);

  rfSource->Write("C0");

  rfSource->Write("E0");
  rfSource->Write("r0");

  rfSource->Write("C1");

  rfSource->Write("E0");
  rfSource->Write("r0");

  return EXIT_SUCCESS;
}
