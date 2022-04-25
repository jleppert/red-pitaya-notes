CFLAGS = -O3 -march=armv7-a -mtune=cortex-a9 -mfpu=neon -mfloat-abi=hard

all: adc-recorder

adc-recorder: adc-recorder.c
	g++ $(CFLAGS) -o $@ $^ -lm -lCppLinuxSerial

clean:
	rm -rf adc-recorder
