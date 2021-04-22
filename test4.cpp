#define THREAD_NUMBER 10
#define ELEMENT_NUMBER 1000


#include "hash.hpp"
#include <iostream>
#include <functional>

std::atomic<bool> start;

void routine(HashTable<int>& ht, int begin, int end)
{
	while(!start);

	for (int i = begin; i < end; ++i)
	{
		while(!ht.add(i, i));
	}
}


int main()
{
	start = false;
	HashTable<int> ht;

	std::vector<std::thread> vec;
	for (int i = 0; i < THREAD_NUMBER; ++i)
	{
		vec.push_back(std::thread(routine, std::ref(ht), i * ELEMENT_NUMBER / THREAD_NUMBER, (i + 1) * ELEMENT_NUMBER / THREAD_NUMBER));
	}

	start = true;

	for (int i = 0; i < THREAD_NUMBER; ++i)
	{
		vec[i].join();
	}	

	ht.print();
	return 0;
}