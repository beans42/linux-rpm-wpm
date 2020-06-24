//after build change binary name to dummy.out so the reader can get its process id
#include <iostream>
#include <thread>

struct data_t {
	uint8_t pattern[12];
	int number;
};

int main() {
	data_t data = { { 0x3A, 0x32, 0x3D, 0x05, 0x2D, 0x40, 0x41, 0x2B, 0x5E, 0x7A, 0xCA, 0xAC }, 0 };
	while (true) {
		printf("%d\n", data.number);
		data.number++;
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}