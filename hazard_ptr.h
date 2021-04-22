enum constants
{
	NODES = 10,
	DLIST_SIZE = 20,
	MAX_HPS = 20
};

struct HazardPointers
{
	std::atomic<void*> nodes[NODES];
	std::atomic <std::thread::id> id;
	unsigned count;
};

const unsigned max_hazard_pointers = MAX_HPS;
HazardPointers hazard_pointers[max_hazard_pointers];

class hp_owner
{
private:
	HazardPointers* hp;
public:
	hp_owner()
	{
		for(unsigned i = 0; i < max_hazard_pointers; ++i)
		{
			std::thread::id old_id;
			if (hazard_pointers[i].id.compare_exchange_strong(old_id, std::this_thread::get_id()))
			{
				hp = &hazard_pointers[i];
				return;
			}
		}
		throw std::runtime_error("No hazard pointers available");
	}

	HazardPointers* get_pointers()
	{
		return hp;
	}

	~hp_owner()
	{
		for (int i = 0; i < NODES; ++i)
		{
			hp->nodes[i] = nullptr;
		}
		hp->id.store(std::thread::id());
	}
};

HazardPointers* get_hazard_pointers_current_thread()
{
	thread_local static hp_owner hazard;
	return hazard.get_pointers();
}



bool find_hp(void* data)
{
	std::thread::id tmp;
	for (int i = 0; i < max_hazard_pointers; ++i)
	{
		if (hazard_pointers[i].id == tmp || hazard_pointers[i].id == std::this_thread::get_id())
			continue;
		for (int j = 0; j < NODES; ++j)
		{
			if (data == hazard_pointers[i].nodes[j])
				return true;
		}
	}
	return false;	
}