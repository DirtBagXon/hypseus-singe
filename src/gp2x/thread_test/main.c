#include "SDL.h"
#include "SDL_thread.h"

int mythread(void *blah)
{
	printf("Thread started...\n");
	printf("Thread stopped...\n");
}

int main(int argc, char **argv)
{
	printf("Main thread is here...\n");
	SDL_Thread *thread = SDL_CreateThread(&mythread, 0);
	sleep(3);
	printf("Main thread exiting...\n");
	return 0;
}
