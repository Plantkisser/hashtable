#include "hash.hpp"
#include <iostream>

int main()
{
	HashTable<int> ht;

	for(int i = 0; i < 40; ++i)
	{
		if (!ht.add(i, i))
			std::cout << i << " ";
	}

	std::cout << ht.get_data(21) << std::endl;
	ht.change_data(22, 21);
	std::cout << ht.get_data(21) << std::endl;
}