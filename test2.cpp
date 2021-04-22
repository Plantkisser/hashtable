#include "hash.hpp"
#include <iostream>

int main()
{
	HashTable<int> ht;

	for(int i = 0; i < 40; ++i)
		ht.add(i, i);

	for(int i = 0; i < 20; ++i)
		ht.remove(i);
	ht.print();

	ht.remove(50);


	ht.print();
}