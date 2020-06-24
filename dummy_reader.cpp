#include <thread>
#include <string>

#include <sys/uio.h>

pid_t process_id;

struct section_t { uintptr_t base, size; };

pid_t get_process_id_from_name(const char* process_name) {
	char command[0x100];
	sprintf(command, "pidof -s %s", process_name);
	auto cmd = popen(command, "r");
	
	char line[0x400];
	fgets(line, sizeof(line), cmd);
	pclose(cmd);
	return strtoul(line, NULL, 10);
}

section_t get_process_stack_base_and_size(const pid_t process_id) {
	char command[0x100];
	sprintf(command, "fgrep '[stack]' /proc/%d/maps", process_id);
	
	char line[0x400];
	auto cmd = popen(command, "r");
	fgets(line, sizeof(line), cmd);
	pclose(cmd);

	uintptr_t base, end;
	sscanf(line, "%llx-%llx", &base, &end);
	return {base, end - base};
}

template<typename T>
T RPM(const uintptr_t address) {
	T buffer;
	iovec local =  { &buffer, sizeof(buffer) };
	iovec remote = { (void*)address, sizeof(buffer) };
	auto num_read = process_vm_readv(process_id, &local, 1, &remote, 1, 0);
	return num_read == sizeof(buffer) ? buffer : T();
}

template<typename T>
void WPM(const uintptr_t address, T buffer) {
	iovec local =  { &buffer, sizeof(buffer) };
	iovec remote = { (void*)address, sizeof(buffer) };
	process_vm_writev(process_id, &local, 1, &remote, 1, 0);
}

void read_bytes(uint8_t* buffer, const uintptr_t address, const size_t length) {
	iovec local =  { buffer, length };
	iovec remote = { (void*)address, length };
	process_vm_readv(process_id, &local, 1, &remote, 1, 0);
}

//modified version of https://github.com/learn-more/findpattern-bench/blob/4ff34fc58cf094356f100bab301d81679e3e4c84/patterns/learn_more.h#L15
uintptr_t find_pattern(uint8_t* start, size_t length, const char* pattern) {
	const char* pat = pattern;
	uintptr_t first_match = 0;
	for (auto current_byte = start; current_byte < start + length; ++current_byte) {
		if (!*pat) return first_match;
		if (*pat == '?' || *current_byte == strtoul(pat, NULL, 16)) {
			if (!first_match) first_match = (uintptr_t)current_byte;
			if (!pat[2]) return first_match;
			pat += *(uint16_t*)pat == 16191 /*??*/ || *pat != '?' ? 3 : 2;
		} else { pat = pattern; first_match = 0; }
	} return 0x0;
}

int main() {
	//get process id of dummy
	process_id = get_process_id_from_name("dummy.out");
	//use process id to get stack pointer and size
	const auto stack_info = get_process_stack_base_and_size(process_id);

	//allocate and copy entire stack of dummy into local address space
	auto bytes = new uint8_t[stack_info.size]();
	read_bytes(bytes, stack_info.base, stack_info.size);

	//find the pattern, translate into the address space of dummy, and add 'number' offset
	const auto address = find_pattern(bytes, stack_info.size, "3A 32 3D 05 2D 40 41 2B 5E 7A CA AC") - (uintptr_t)bytes + stack_info.base + 0xC;

	//delete local copy of duumy's stack
	delete[] bytes;

	//manipulate dummy's data
	while (true) {
		const auto num = RPM<int>(address);
		WPM(address, num + 9);
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}
