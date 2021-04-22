#define THREAD_NUMBER 10
#define ELEMENT_NUMBER 1000

#include "hash.hpp"
#include <iostream>
#include <functional>

#include <unistd.h>

std::atomic<bool> start_add;
std::atomic<int> start_delete;


void routine_delete(HashTable<int>& ht, int begin, int end)
{
	while(start_delete < THREAD_NUMBER);

	for (int i = begin; i < end; ++i)
	{
		ht.remove(i);
	}
}




void routine_add(HashTable<int>& ht, int begin, int end)
{
	while(!start_add);

	for (int i = begin; i < end; ++i)
	{
		//if (i == (end-begin)/2 + begin)
			//start_delete++;

		while(!ht.add(i, i));
	}
	start_delete++;
}


int main()
{
	//std::cout << getpid() << std::endl;

	start_add = false;
	start_delete = 0;
	HashTable<int> ht;

	std::vector<std::thread> vec_add;
	std::vector<std::thread> vec_delete;
	for (int i = 0; i < THREAD_NUMBER; ++i)
	{
		vec_add.push_back(std::thread(routine_add, std::ref(ht), i * ELEMENT_NUMBER / THREAD_NUMBER, (i + 1) * ELEMENT_NUMBER / THREAD_NUMBER));
	}

	for (int i = 0; i < THREAD_NUMBER; ++i)
	{
		vec_delete.push_back(std::thread(routine_delete, std::ref(ht), i * ELEMENT_NUMBER / THREAD_NUMBER, (i + 1) * ELEMENT_NUMBER /(THREAD_NUMBER) - ELEMENT_NUMBER /(THREAD_NUMBER)/2));
	}

	start_add = true;

	for (int i = 0; i < THREAD_NUMBER; ++i)
	{
		vec_add[i].join();
		vec_delete[i].join();
	}	

	ht.print();
	return 0;
}