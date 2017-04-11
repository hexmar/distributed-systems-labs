#include <iostream>
#include <chrono>
#include <mutex>
#include <thread>
#include <atomic>
#include <random>

using namespace std;

volatile double summ = 0;
volatile atomic_flag summFlag = ATOMIC_FLAG_INIT;

int main(int argc, char **argv)
{
	if (argc != 4) {
		cout << "Error: Incorrect parameters. Usage: <producers> <consumers> <buffer size>\n";
		return EXIT_FAILURE;
	}
	unsigned int prods = atoi(argv[1]), cons = atoi(argv[2]), buffSize = atoi(argv[3]);
	cout << "Start program with "
		<< prods << " producers, "
		<< cons << " consumers, buffer size = "
		<< buffSize << endl;
	// Random generator
	random_device rd;
	default_random_engine eng(rd());
	uniform_int_distribution<int> dist(1, 9);
	int numberOfOperations;
	{
		uniform_int_distribution<int> dist(2, 5);
		numberOfOperations = dist(eng);
	}
	volatile atomic_uchar flags[buffSize];
	volatile double data[buffSize];
	for (int i = 0; i < buffSize; ++i)
		flags[i] = 0;
	auto readerWork = [&flags, &data, &summ, buffSize, &summFlag] (int numberOfOperations) {
		unsigned char flag = 2;
		double localSumm = 0;
		for (unsigned int i = 0; true; i = (i + 1) == buffSize ? 0 : i + 1) {
			if (numberOfOperations == 0)
				break;
			if (flags[i].compare_exchange_weak(flag, (unsigned char) 1, memory_order_acquire)) {
				localSumm += data[i];
				flags[i].store(0, memory_order_release);
				--numberOfOperations;
			}
		}
		while (summFlag.test_and_set(memory_order_acquire));
		summ += localSumm;
		summFlag.clear(memory_order_release);
	};
	auto writerWork = [&flags, &data, buffSize, &dist, &eng] (int numberOfOperations) {
		unsigned char flag = 0;
		for (unsigned int i = 0; true; i = (i + 1) == buffSize ? 0 : i + 1) {
			if (numberOfOperations == 0)
				break;
			if (flags[i].compare_exchange_weak(flag, (unsigned char) 1, memory_order_acquire)) {
				data[i] = dist(eng);
				flags[i].store(2, memory_order_release);
				--numberOfOperations;
			}
		}
	};
	thread *writers[prods];
	thread *readers[cons];
	for (int i = 0; i < prods; ++i)
		writers[i] = new thread(writerWork, numberOfOperations);
	for (int i = 0; i < cons; ++i)
		readers[i] = new thread(readerWork, numberOfOperations);
	for (int i = 0; i < prods; ++i) {
		writers[i]->join();
		delete writers[i];
	}
	for (int i = 0; i < cons; ++i) {
		readers[i]->join();
		delete readers[i];
	}
	cout << "\nSumm = " << summ << endl;
	return 0;
}

