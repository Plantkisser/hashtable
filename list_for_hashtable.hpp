
#include <thread>
#include <atomic>
#include <iostream>

#include <unistd.h>
#include <time.h>

 
#define MASK 0xfffffffffffffffe
#include "hazard_ptr.h"



template <typename T>
struct Node
{
	std::atomic<Node<T>*> next;
	int key;
	T data;
	bool is_sential;
};



template <typename T>
class List
{
private:
	//std::recursive_mutex debug_mutex;
	std::atomic<Node<T>*> head_;
	std::atomic<Node<T>*> tail_;
public:
	Node<T>* insert(const T*, int, bool, Node<T>*, Node<T>*);
	bool remove(int key, Node<T>*, Node<T>*);
	void print();

	// this two function will throw exception in case there is no element with this key
	T get_data(int key, Node<T>*, Node<T>*);
	void change_data(const T&, int, Node<T>*, Node<T>*);

	Node<T>* begin();
	Node<T>* end();


	List();
	~List();
};





bool compare_less(int left_key, bool is_left_sential, int right_key, bool is_right_sential)
{
	unsigned inv_left_key = 0;
	unsigned inv_right_key = 0;

	while(right_key > 0 || left_key > 0)
	{
		inv_right_key += right_key & 1;
		right_key >>= 1;
		inv_right_key <<= 1;

		inv_left_key += left_key & 1;
		left_key >>= 1;
		inv_left_key <<= 1;		
	}

	if (!is_left_sential)
		inv_left_key += 1;
	if (!is_right_sential)
		inv_right_key += 1;

	return inv_left_key < inv_right_key;

}






std::atomic <int> dcount;
void* dlist[DLIST_SIZE];
template <typename T>
bool retire(void* data)
{
	std::thread::id tmp;
	bool is_read = find_hp(data);
	int n = 0;	
	if (!is_read)
	{	
		delete (static_cast<T*> (data));
	}
	else
	{
		do
		{
			n = dcount.load();
			if (n == DLIST_SIZE)
				throw(std::runtime_error("dlist full"));
		} while(!dcount.compare_exchange_strong(n, n+1)); 
		dlist[n+1] = data;
	}


	if (n+1 == DLIST_SIZE)
	{
		for(int i = 0; i < DLIST_SIZE; ++i)
		{
			if (!find_hp(dlist[i]))
			{
				delete (static_cast<T*> (dlist[i]));
				dlist[i] = nullptr;
			}
		}
	}
	return true;
}












template <typename T>
Node<T>* List<T>:: insert(const T* item, int key, bool is_node_sential, Node<T>* start_pointer, Node<T>* end_pointer)
{
	HazardPointers* hps = get_hazard_pointers_current_thread();

	Node<T>* item_ptr = new(Node<T>);
	item_ptr->key = key;
	if (!is_node_sential)
		item_ptr->data = *item;
	item_ptr->is_sential = is_node_sential;


	hps->nodes[0].store(start_pointer);
	hps->nodes[1].store((Node<T>*)((long)start_pointer->next.load() & MASK));

	Node<T>* prev = (Node<T>*) hps->nodes[0].load();
	Node<T>* curr = (Node<T>*) hps->nodes[1].load(); 

	Node<T>* retval = nullptr;
	
	while(1)
	{

		while (curr != end_pointer && compare_less(curr->key, curr->is_sential, key, is_node_sential))//curr->key < key)
		{
			hps->nodes[0].store(curr);
			hps->nodes[1].store((Node<T>*) ((long)curr->next.load() & MASK));

			prev = curr;
			curr = (Node<T>*) (hps->nodes[1].load());
		}

		if (curr != end_pointer && key == curr->key && !curr->is_sential && !is_node_sential)
		{
			retval = nullptr;
			break;
		}

		if ((long)(prev->next.load()) & 1)
		{
			hps->nodes[0] = start_pointer;
			hps->nodes[1].store((Node<T>*) ((long)start_pointer->next.load() & MASK));

			Node<T>* prev = start_pointer;
			Node<T>* curr = (Node<T>*) ((long)start_pointer->next.load() & MASK);
			continue;
		}


		item_ptr->next = curr;
		if (prev->next.compare_exchange_strong(curr, item_ptr))
		{
			retval = item_ptr;
			break;
		}
	}

	hps->nodes[0].store(nullptr);
	hps->nodes[1].store(nullptr);	
	return retval;
}



template <typename T>
bool List<T>:: remove(int key, Node<T>* start_pointer, Node<T>* end_pointer)
{
	HazardPointers* hps = get_hazard_pointers_current_thread();

	while(1)
	{

		hps->nodes[0].store(start_pointer);
		hps->nodes[1].store((Node<T>*)((long)start_pointer->next.load() & MASK));


		Node<T>* prev = (Node<T>*) hps->nodes[0].load();
		Node<T>* curr = (Node<T>*) hps->nodes[1].load();

		while (curr != end_pointer && compare_less(curr->key, curr->is_sential, key, false)) //curr->key < key)
		{
			hps->nodes[0] = curr;
			hps->nodes[1] = (Node<T>*)((long)(curr->next.load()) & MASK);

			curr = (Node<T>*) (hps->nodes[1].load());
			prev = (Node<T>*) (hps->nodes[0].load());
		}

		if (curr == end_pointer || curr->key != key)
		{
			hps->nodes[0] = nullptr;
			hps->nodes[1] = nullptr;
			return false;
		}




		
		Node<T>* ptr = curr->next;
		if ((long)ptr & (long)1)
		{
			hps->nodes[0] = nullptr;
			hps->nodes[1] = nullptr;
			return false;
		}
		if (curr->next.compare_exchange_strong(ptr, (Node<T>*) ((long)ptr | 1)))
		{
			Node<T>* tmp = curr;
			ptr =  (Node<T>*)((long)ptr & MASK);
			if (!prev->next.compare_exchange_strong(tmp, ptr))
			{
				curr->next = (Node<T>*)((long) curr->next.load() & MASK);
				continue;
			}
				

			try
			{
				retire<T>(curr);
			}
			catch(const std::exception& exc)
			{
				hps->nodes[0] = nullptr;
				hps->nodes[1] = nullptr;		
				std::cout << exc.what() << std::endl;
				throw;
			}
			catch(...)
			{
				hps->nodes[0] = nullptr;
				hps->nodes[1] = nullptr;
				std::cout << "Unexpected exception" << std::endl;
				throw;
			}
			hps->nodes[0] = nullptr;
			hps->nodes[1] = nullptr;
			return true;
		
		}
	}
}


template <typename T>
Node<T>* List<T>:: begin()
{
	return head_.load();
}

template <typename T>
Node<T>* List<T>:: end()
{
	return tail_.load();
}



template <typename T>
T List<T>:: get_data(int key, Node<T>* start_pointer, Node<T>* end_pointer)
{
	HazardPointers* hps = get_hazard_pointers_current_thread();

	
	hps->nodes[0].store(start_pointer);
	hps->nodes[1].store((Node<T>*)((long)start_pointer->next.load() & MASK));


	Node<T>* prev = (Node<T>*) hps->nodes[0].load();
	Node<T>* curr = (Node<T>*) hps->nodes[1].load();

	while (curr != end_pointer && compare_less(curr->key, curr->is_sential, key, false)) //curr->key < key)
	{

		hps->nodes[0] = curr;
		hps->nodes[1] = (Node<T>*)((long)(curr->next.load()) & MASK);

		curr = (Node<T>*) (hps->nodes[1].load());
		prev = (Node<T>*) (hps->nodes[0].load());
	}

	if (curr == end_pointer || curr->key != key)
	{
		hps->nodes[0] = nullptr;
		hps->nodes[1] = nullptr;
		throw(std::runtime_error("No item with this key"));
	}

	T retval = curr->data;

	hps->nodes[0] = nullptr;
	hps->nodes[1] = nullptr;

	return retval;
}




template <typename T>
void List<T>:: change_data(const T& item, int key, Node<T>* start_pointer, Node<T>* end_pointer)
{
	HazardPointers* hps = get_hazard_pointers_current_thread();


	hps->nodes[0].store(start_pointer);
	hps->nodes[1].store((Node<T>*)((long)start_pointer->next.load() & MASK));


	Node<T>* prev = (Node<T>*) hps->nodes[0].load();
	Node<T>* curr = (Node<T>*) hps->nodes[1].load();

	while (curr != end_pointer && compare_less(curr->key, curr->is_sential, key, false)) //curr->key < key)
	{

		hps->nodes[0] = curr;
		hps->nodes[1] = (Node<T>*)((long)(curr->next.load()) & MASK);

		curr = (Node<T>*) (hps->nodes[1].load());
		prev = (Node<T>*) (hps->nodes[0].load());
	}
	
	if (curr == end_pointer || curr->key != key)
	{
		hps->nodes[0] = nullptr;
		hps->nodes[1] = nullptr;
		throw(std::runtime_error("No item with this key"));
	}

	curr->data = item;

	hps->nodes[0] = nullptr;
	hps->nodes[1] = nullptr;
}





template <typename T>
void List<T>:: print()
{
	Node<T>* ptr;
	ptr = (Node<T>*)((long)head_.load()->next.load() & MASK);
	unsigned count = 0;
	while(ptr != tail_.load())
	{
		if (ptr->is_sential)
			std::cout << "'" << ptr->key << "'" << " ";
		else
			std::cout << ptr->key << " ";
		
		ptr = (Node<T>*)((long)ptr->next.load() & MASK);
		count++;
	}
	std::cout << std::endl << "Total: " << count << std::endl;
}


template <typename T>
List<T>:: List()
{
	for(int i = 0; i < DLIST_SIZE; ++i)
	{
		dlist[i] = nullptr;
	}

	head_ = new(Node<T>);
	tail_ = new(Node<T>);

	head_.load()->next.store(tail_.load());
	tail_.load()->key = -1;
}

template <typename T>
List<T>:: ~List()
{
	Node<T>* curr = head_;

	while(curr != tail_)
	{
		Node<T>* next = (Node<T>*)((long)curr->next.load() & MASK);
		retire<T>(curr);
		curr = next;
	}
	retire<T>(tail_);

	for(int i = 0; i < DLIST_SIZE; ++i)
	{
		if (dlist[i])
			delete (static_cast<T*> (dlist[i]));  //might cause sigfault
	}
}


