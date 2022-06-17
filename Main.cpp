#include <algorithm>
#include <iostream>
#include <random>
#include <queue>
#include <list>

#define PROCESS_MAX_LENGTH UINT16_MAX
#define PROCESS_MAX_ID UINT32_MAX

#define PROCESS_STATE_READY 0Ui8
#define PROCESS_STATE_RUNNING 1Ui8
#define PROCESS_STATE_BLOCKED 2Ui8

class Random {
public:
	// Initializam motorul de generare cu samanta
	Random() {
		mt.seed(rd());
	}
	~Random() {}

	//  Functia ce genereaza numarul intreg aleatoriu
	inline uint32_t GetUInt32(const uint32_t min, const uint32_t max) {
		return std::uniform_int_distribution<std::mt19937::result_type>(min, max)(mt);
	}

private:
	std::random_device rd; // dispozitivul care ne va da samanta
	std::mt19937 mt; // motorul cu care generam
};

class Process {
public:
	Process() {}
	Process(const uint32_t timeStart, const uint32_t timeLeft, const uint32_t id, const uint32_t ram, const uint8_t state = PROCESS_STATE_READY):
		timeStart(timeStart), timeLeft(timeLeft), id(id), ram(ram), state(state) {}
	~Process() {}

	virtual void OnCreate() {
		std::cout << "Process of id " << id << " has been created!" << std::endl;
	}

	virtual void OnRunBefore() {
		//std::cout << "Process of id " << id << " will run!" << std::endl;
	}

	virtual void OnRunAfter() {
		//std::cout << "Process of id " << id << " finished running!" << std::endl;
	}

	virtual void OnDestroy() {
		std::cout << "Process of id " << id << " has been destroyed!" << std::endl;
	}

	uint32_t timeStart = 0Ui32, timeLeft = 0Ui16;
	uint32_t id = 0Ui16, ram = 0Ui32;
	uint8_t state = PROCESS_STATE_READY; // 0 -> READY, 1 -> RUNNING, 2 -> BLOCKED
};

class ProcessScheduler {
public:
	ProcessScheduler() {}
	ProcessScheduler(const uint32_t timeTotal = 6000000Ui32, const uint8_t timeQuantum = 50Ui8):
		timeTotal(timeTotal), timeQuantum(timeQuantum) {}
	~ProcessScheduler() {}

	inline void ProcessAdd(const Process& process) {
		processes.push(process);
		processes.back().OnCreate();
	}

	inline Process ProcessGenerateRandom(const uint32_t timeStart, const uint32_t timeLeft = 0Ui32, const uint32_t id = 0Ui32, const uint32_t ram = 0Ui32, const uint8_t state = PROCESS_STATE_READY) {
		return Process {
			timeStart,
			timeLeft ? timeLeft : random.GetUInt32(10000Ui32, PROCESS_MAX_LENGTH),
			id ? id : random.GetUInt32(0Ui32, PROCESS_MAX_ID),
			ram ? ram : random.GetUInt32(1Ui32, 1000000Ui32),
			state
		};
	}

	void CheckNextTimeQuantum() {
		// Daca in urmatoarea cuanta de timp avem de momente de creat procese le creem acusica
		while(!processesMoments.empty() &&
			  processesMoments.back() >= timeCurrent &&
			  processesMoments.back() <= timeCurrent + timeQuantum) {
			ProcessAdd(ProcessGenerateRandom(processesMoments.back()));
			processesMoments.pop_back();
		}
	}

	inline void ProcessAddMoment(const uint32_t timeStart) {
		// Adaugam o valoare in vectorul de momente in care sa se creeze procese sortat
		processesMoments.insert(std::lower_bound(processesMoments.begin(), processesMoments.end(), timeStart, std::greater<int32_t>()), timeStart);
	}

	void ProcessSwap() {
		if(processes.empty()) {
			return;
		}

		// Daca mai trebuie sa ruleze il rebagam in coada, altfel ii apelam evenimentul de distrugere si la final scoatem din coada procesul
		if(processes.front().timeLeft > 0Ui32) {
			processes.push(processes.front());
		} else {
			processes.front().OnDestroy();
		}

		processes.pop();
	}

	uint8_t ProcessRun() {
		uint8_t timeElapsed = timeQuantum;
		// Daca coada este goala si daca suntem intr-un rest de cuanta
		if(processes.empty()) {
			if(timeQuantumResidue) {
				timeElapsed = timeQuantumResidue;
				timeQuantumResidue = 0Ui32;
			}

			return timeElapsed;
		}

		// Actualizam procesul inainte sa ruleze
		processes.front().OnRunBefore();
		processes.front().state = PROCESS_STATE_RUNNING;

		// Folosim restul de cuanta de timp
		if(timeQuantumResidue) {
			if(timeQuantumResidue > processes.front().timeLeft) {
				timeElapsed = processes.front().timeLeft;
				timeQuantumResidue -= processes.front().timeLeft;
			} else {
				timeElapsed = timeQuantumResidue;
				timeQuantumResidue = 0Ui32;
			}
		} else {
			if(timeQuantum > processes.front().timeLeft) {
				timeQuantumResidue = timeQuantum - processes.front().timeLeft;
				timeElapsed = processes.front().timeLeft;
			}
		}

		// Actualizam procesul dupa ce a rulat
		processes.front().OnRunAfter();

		processes.front().timeLeft -= timeElapsed;
		processes.front().state = PROCESS_STATE_READY;

		return timeElapsed;
	}

	void Simulate() {
		while(timeCurrent < timeTotal) {
			// Verificam daca suntem la finalul undei cuante
			if(timeCurrent % timeQuantum == 0Ui32) {
				// Se creeaza un moment de proces la fiecare 18s
				if(timeCurrent % 18000 == 0) {
					ProcessAddMoment(random.GetUInt32(timeCurrent, timeCurrent + 1000000i32));
				}

				CheckNextTimeQuantum();
			}

			uint8_t timeElapsed = ProcessRun();
			timeCurrent += timeElapsed;

			// Facem schimbarea de procese daca ultimul proces a rulat cel putin jumatate de cuanta
			// (Daca restul de cuanta era 10 din 50 atunci rula prea putin, astfel incat o sa ia si urmatoarea cuanta de timp)
			//if(timeElapsed >= timeQuantum / 2Ui8) {
			ProcessSwap();
			//}
		}
	}

private:
	uint32_t timeTotal = 0Ui32, timeCurrent = 0Ui32;
	uint8_t timeQuantum = 0Ui8, timeQuantumResidue = 0Ui8;
	std::list<uint32_t> processesMoments;
	std::queue<Process> processes;

	Random random;
};

int main() {
	ProcessScheduler(6000000Ui32, 50Ui32).Simulate();

	return 0Ui32;
}