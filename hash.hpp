#include "list_for_hashtable.hpp"
#include <vector>
#include <cmath>


template <typename T> 
class HashTable
{
private:
	List<T> list_;
	std::vector<void*> sential_nodes_;
	std::atomic<unsigned> size_;
	std::atomic<unsigned> sential_nodes_number_;

	unsigned k; // max size will be calculated as k * sential_nodes_number_

	int hash(int key);
public:
	HashTable();
	~HashTable() = default;

	bool add(const T& data, int key);
	bool remove(int key);

	T get_data(int key);
	void change_data(const T& data, int key);

	void print();
};







int get_next_bucket(int bucket, int sential_nodes_number)
{
	if (sential_nodes_number - 1 == bucket)
		return -1;

	while(1)
	{
		if (bucket < sential_nodes_number / 2)
			return bucket + sential_nodes_number / 2;
		sential_nodes_number /= 2;
		bucket -= sential_nodes_number;
	}
}


template <typename T> 
bool HashTable<T>:: add(const T& data, int key)
{
	int hash_key = hash(key);

	unsigned size = size_;
	do
	{
		size = size_;
		unsigned max_size = k * sential_nodes_number_;
		if (size >= max_size)
		{
			return false;
		}
	} while(!size_.compare_exchange_strong(size, size + 1));

	while(1)
	{
		unsigned sential_nodes_number = sential_nodes_number_; //to prevent changing during work with sential_nodes_number_
		int bucket = hash_key % sential_nodes_number;
		int next_bucket = get_next_bucket(bucket, sential_nodes_number);

		void* start_pointer = sential_nodes_[bucket];
		void* end_pointer =(next_bucket == -1)? list_.end() : sential_nodes_[next_bucket];
		if (start_pointer)
		{
			if (list_.insert(&data, hash_key, false, (Node<T>*)start_pointer, (Node<T>*)end_pointer))
			{	
				break;
			}
			else
			{
				size = size_;
				while(!size_.compare_exchange_strong(size, size - 1));
				return true;
			}
		}
	}


	/*
	
	* Rehash part if size become equal to max size of hash table

	*/
	if (size + 1 == k * sential_nodes_number_)
	{
		unsigned new_sential_nodes_number = sential_nodes_number_ * 2;

		for (int i = sential_nodes_number_; i < new_sential_nodes_number; ++i)
		{
			void* ptr = list_.insert(nullptr, i, true, (Node<T>*) sential_nodes_[i - sential_nodes_number_], list_.end());
			if (!ptr)
			{
				throw(std::runtime_error("Cannot insert new sential node"));
			}
			sential_nodes_.push_back(ptr);
		}

		sential_nodes_number_ = new_sential_nodes_number;
	}

	return true;
}

template <typename T> 
bool HashTable<T>:: remove(int key)
{
	int hash_key = hash(key);

	unsigned sential_nodes_number = sential_nodes_number_;
	
	int bucket = hash_key % sential_nodes_number;
	int next_bucket = get_next_bucket(bucket, sential_nodes_number);

	void* start_pointer = sential_nodes_[bucket];
	void* end_pointer =(next_bucket == -1)? list_.end() : sential_nodes_[next_bucket];

	if (list_.remove(hash_key, (Node<T>*) start_pointer, (Node<T>*) end_pointer))
	{
		unsigned size = size_;
		while(!size_.compare_exchange_strong(size, size - 1));
		return true;
	}
	else
	{
		return false;
	}
}

template <typename T> 
T HashTable<T>:: get_data(int key)
{
	int hash_key = hash(key);

	unsigned sential_nodes_number = sential_nodes_number_;
	
	int bucket = hash_key % sential_nodes_number;
	int next_bucket = get_next_bucket(bucket, sential_nodes_number);

	void* start_pointer = sential_nodes_[bucket];
	void* end_pointer =(next_bucket == -1)? list_.end() : sential_nodes_[next_bucket];

	return list_.get_data(key, (Node<T>*) start_pointer, (Node<T>*) end_pointer);
}


template <typename T> 
void HashTable<T>:: change_data(const T& data, int key)
{
	int hash_key = hash(key);

	unsigned sential_nodes_number = sential_nodes_number_;
	
	int bucket = hash_key % sential_nodes_number;
	int next_bucket = get_next_bucket(bucket, sential_nodes_number);

	void* start_pointer = sential_nodes_[bucket];
	void* end_pointer =(next_bucket == -1)? list_.end() : sential_nodes_[next_bucket];

	return list_.change_data(data, key, (Node<T>*) start_pointer, (Node<T>*) end_pointer);
}



template <typename T> 
void HashTable<T>:: print()
{
	list_.print();
}



template <typename T>
int HashTable<T>:: hash(int key)
{
	return key;
}

template <typename T> 
HashTable<T>:: HashTable():
size_(0),
sential_nodes_number_(2),
k(5)
{
	void* ptr = nullptr;

	ptr = list_.insert(nullptr, 0, true, list_.begin(), list_.end());
	sential_nodes_.push_back(ptr);

	ptr = list_.insert(nullptr, 1, true, list_.begin(), list_.end());
	sential_nodes_.push_back(ptr);;
}





