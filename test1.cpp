#include "hash.hpp"
#include <iostream>

int main()
{
	HashTable<int> ht;
	ht.print();

	for(int i = 0; i < 40; ++i)
		ht.add(i, i);

	ht.print();

}