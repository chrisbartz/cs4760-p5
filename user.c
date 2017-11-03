//Christopher Bartz
//cyb01b
//CS4760 S02
//Project 5

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/wait.h>
#include "sharedMemory.h"
#include "timestamp.h"
#include "queue.h"

#define DEBUG 1 			// setting to 1 greatly increases number of logging events
#define TUNING 0
#define MAX_WORK_INTERVAL 75 * 1000 * 1000 // max time to work
#define BINARY_CHOICE 2
#define MAX_RESOURCE_WAIT 100
#define MAX_LUCKY_NUMBER 50

SmStruct shmMsg;
SmStruct *p_shmMsg;

int childId; 				// store child id number assigned from parent
int pcbIndex;				// store index of pcb

int startSeconds;			// store oss seconds when initializing shared memory
int startUSeconds;			// store oss nanoseconds when initializing shared memory
int endSeconds;				// store oss seconds to exit
int endUSeconds;			// store oss nanoseconds to exit
int exitSeconds;			// store oss seconds when exiting
int exitUSeconds;			// store oss nanoseconds when exiting

int userWaitSeconds;		// the next time the user process makes a resource decision
int userWaitUSeconds;		// the next time the user process makes a resource decision
int luckyNumber = rand() % MAX_LUCKY_NUMBER; // a random number to determine if the process terminates
int requestedAResource = 0;

char timeVal[30]; // formatted time values for logging

void do_work(int willRunForThisLong);

void increment_user_clock_values(int ossSeconds, int ossUSeconds, int seconds, int uSeconds, int offset);

int main(int argc, char *argv[]) {
childId = atoi(argv[0]); // saves the child id passed from the parent process
pcbIndex = atoi(argv[1]); // saves the pcb index passed from the parent process

getTime(timeVal);
if (DEBUG) printf("user %s: PCBINDEX: %d\n", timeVal, pcbIndex);

srand(getpid()); // random generator
int processTimeRequired = rand() % (MAX_WORK_INTERVAL);
const int oneBillion = 1000000000;

// a quick check to make sure user received a child id
getTime(timeVal);
if (childId < 0) {
	if (DEBUG) printf("user %s: Something wrong with child id: %d\n", timeVal, getpid());
	exit(1);
} else {
	if (DEBUG) printf("user %s: child %d (#%d) simulated work load: %d started normally after execl\n", timeVal, (int) getpid(), childId, processTimeRequired);

	// instantiate shared memory from oss
	getTime(timeVal);
	if (DEBUG) printf("user %s: child %d (#%d) create shared memory\n", timeVal, (int) getpid(), childId);

	// refactored shared memory using struct
	int shmid;
	if ((shmid = shmget(SHM_MSG_KEY, SHMSIZE, 0660)) == -1) {
		printf("sharedMemory: shmget error code: %d", errno);
		perror("sharedMemory: Creating shared memory segment failed\n");
		exit(1);
	}
	p_shmMsg = &shmMsg;
	p_shmMsg = shmat(shmid, NULL, 0);

	startSeconds = p_shmMsg->ossSeconds;
	startUSeconds = p_shmMsg->ossUSeconds;

	getTime(timeVal);
	if (TUNING || DEBUG)
		printf("user %s: child %d (#%d) read start time in shared memory: %d.%09d\n",
			timeVal, (int) getpid(), childId, startSeconds, startUSeconds);


	// open semaphore
	sem_t *sem = open_semaphore(0);

	struct timespec timeperiod;
	timeperiod.tv_sec = 0;
	timeperiod.tv_nsec = 5 * 10000;

	increment_user_clock_values(p_shmMsg->ossSeconds, p_shmMsg->ossUSeconds, userWaitSeconds, userWaitUSeconds, ( rand() % MAX_RESOURCE_WAIT));

	while (1) { // main while loop

		// make decision about whether to terminate successfully
		if (!(p_shmMsg->ossSeconds >= userWaitSeconds && p_shmMsg->ossUSeconds > userWaitUSeconds)) {
				nanosleep(&timeperiod, NULL); // reduce the cpu load from looping
			} else {
				// make decision about whether to terminate
				if (rand() % MAX_LUCKY_NUMBER == luckyNumber)
					break;
			}

		// check for requested resource
		if (requestedAResource) {
			if (p_shmMsg->userPid != (int) getpid() || p_shmMsg->userGrantedResource == 0) {
				// resource has not been granted (yet)
				continue;
			} else {
				// then resource has been granted
				sem_wait(sem);
				p_shmMsg->userPid = 0;
				p_shmMsg->userRequestOrRelease = 0;
				p_shmMsg->userResource = 0;
				p_shmMsg->userGrantedResource = 0;
				sem_post(sem);
				requestedAResource = 0;
			}
		}

		// make decision about whether to request a resource
		if ((rand() % MAX_LUCKY_NUMBER) == luckyNumber) {
			if (rand() % BINARY_CHOICE) {
				// request a resource
				sem_wait(sem);
				p_shmMsg->userPid = getpid();
				p_shmMsg->userRequestOrRelease = 0;
				p_shmMsg->userResource = rand() % MAX_RESOURCE_COUNT;
				sem_post(sem);
				requestedAResource = 1;
			} else {
				// release a resource
				int releasedResource = 0;

				// first find a resource to release
				for (int i = 0; i < 100; i++) {
					if (p_shmMsg->pcb[pcbIndex].resources[i] != 0) {
						releasedResource = p_shmMsg->pcb[pcbIndex].resources[i];
						break;
					}
				}

				// send order to release resource
				if (releasedResource) {
					sem_wait(sem);
					p_shmMsg->userPid = getpid();
					p_shmMsg->userRequestOrRelease = 1;
					p_shmMsg->userResource = releasedResource;
					sem_post(sem);
				}
			}
		}



//			sem_wait(sem);

//			int runTime = p_shmMsg->dispatchedTime; // this is our maximum running time

			// clear the message from oss
//			p_shmMsg->dispatchedPid = 0;
//			p_shmMsg->dispatchedTime = 0;

//			sem_post(sem);

//			getTime(timeVal);
//			printf("user %s: Receiving that process %d can run for %d nanoseconds\n", timeVal, (int) getpid(), runTime);
//
//
//
//
//			int willTerminateSuccessfully = (rand() % BINARY_CHOICE); // make decision whether we will terminate the process
//			int willRunForThisLong;
//
//			if (willRunForFullTime) {
//				willRunForThisLong = runTime; // we will run for the full quantum assigned
//			} else {
//				willRunForThisLong = (rand() % runTime); // determine how long we will run if partial
//			}
//
//			do_work(willRunForThisLong); // doing "work"
//
//			sem_wait(sem);
//
//			// report back to oss
//			p_shmMsg->userPid = (int) getpid();
//			if (p_shmMsg->pcb[pcbIndex].totalCpuTime + willRunForThisLong > processTimeRequired) {
//				p_shmMsg->userHaltSignal = 0; // terminating - send last message
//
//
//			}
//			else
//				p_shmMsg->userHaltSignal = 1; // halting
//			p_shmMsg->userHaltTime = willRunForThisLong;
//
//			sem_post(sem);
//
//			getTime(timeVal);
//			if (DEBUG) printf("user %s: Process %d checking escape conditions\nTotalCPUTime: %d willRunForThisLong: %d processTimeRequired: %d\n", timeVal, (int) getpid(), p_shmMsg->pcb[pcbIndex].totalCpuTime, willRunForThisLong ,processTimeRequired);
//
//			if (p_shmMsg->pcb[pcbIndex].totalCpuTime + willRunForThisLong > processTimeRequired)
//				break;

	} // end main while loop

	getTime(timeVal);
	printf("user %s: Process %d escaped main while loop\n", timeVal, (int) getpid());

	sem_wait(sem);

	// report process termination to oss
	p_shmMsg->userPid = (int) getpid();
	p_shmMsg->userHaltSignal = 0;

	// send total bookkeeping stats
	p_shmMsg->pcb[pcbIndex].startUserSeconds = startSeconds;
	p_shmMsg->pcb[pcbIndex].startUserUSeconds = startUSeconds;
	p_shmMsg->pcb[pcbIndex].endUserSeconds = p_shmMsg->ossSeconds;
	p_shmMsg->pcb[pcbIndex].endUserUSeconds = p_shmMsg->ossUSeconds;

	sem_post(sem);

	// clean up shared memory
	shmdt(p_shmMsg);

	// close semaphore
	close_semaphore(sem);

	getTime(timeVal);
	if (DEBUG) printf("user %s: child %d (#%d) exiting normally\n", timeVal, (int) getpid(), childId);
}
exit(0);
}


// this part should occur within the critical section if
// implemented correctly since it accesses shared resources
void do_work(int willRunForThisLong) {

	getTime(timeVal);
	printf("user %s: Process %d doing work for %d nanoseconds\n", timeVal, (int) getpid(), willRunForThisLong);

	struct timespec sleeptime;
	sleeptime.tv_sec = 0;
	sleeptime.tv_nsec = willRunForThisLong;
	nanosleep(&sleeptime, NULL); // we are doing "work" here

	getTime(timeVal);
	printf("user %s: Process %d done doing work\n", timeVal, (int) getpid());

}

void increment_user_clock_values(int ossSeconds, int ossUSeconds, int seconds, int uSeconds, int offset) {
	const int oneBillion = 1000000000;

	int localOssSeconds = ossSeconds;
	int localOssUSeconds = ossUSeconds;

	localOssUSeconds += offset;

	if (localOssUSeconds >= oneBillion) {
		localOssSeconds++;
		localOssUSeconds -= oneBillion;
	}

	seconds = localOssSeconds;
	uSeconds = localOssUSeconds;

	if (DEBUG)
				printf("user: updating user wait time values by %d ms to %d.%09d\n", offset, ossSeconds, ossUSeconds);
}
