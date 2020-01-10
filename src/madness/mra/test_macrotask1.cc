/*
 * test_macrotask.cc
 *
 *  Created on: Dec 17, 2019
 *      Author: fbischoff
 */
//#define WORLD_INSTANTIATE_STATIC_TEMPLATES

#include <madness/mra/mra.h>
#include <iostream>
#include <madness/world/MADworld.h>
#include <madness/world/worlddc.h>
#include <random>
#include <madness/mra/funcimpl.h>

using namespace madness;
using namespace archive;

/**
 * 	Issues:
 * 	 - set_defaults<>(local_world) for loading (affects pmap)
 * 	 - serialization of task works only for int, double, .. but not for Function
 *   - turn data structure into tuple
 *   - prioritize tasks
 *
 */


/// for each process create a world using a communicator shared with other processes by round-robin
/// copy-paste from test_world.cc
std::shared_ptr<World> create_worlds(World& universe, const std::size_t nworld) {

	if (universe.size()<nworld) {
		print("trying to create ",nworld,"world with",universe.size(), "processes");
		MADNESS_EXCEPTION("increase number of processes",1);
	}

	if (universe.rank()==0) print("== multiple worlds created with Intracomm::Create()==",nworld);
    std::vector<std::vector<int> > process_list(nworld);
    std::shared_ptr<World> all_worlds;

	for (int i=0; i<universe.size(); ++i) process_list[i%nworld].push_back(i);
	if (universe.rank()<nworld) print("process_list",process_list[universe.rank()]);


	for (int i=0; i<process_list.size(); ++i) {
		const std::vector<int>& pl=process_list[i];
		bool found=(std::find(pl.begin(),pl.end(),universe.rank())!=pl.end());
		if (found) {
			print("assigning rank",universe.rank(),"to world group",pl);

			SafeMPI::Group group = universe.mpi.comm().Get_group().Incl(pl.size(), &pl[0]);
			SafeMPI::Intracomm comm_group = universe.mpi.comm().Create(group);

			all_worlds.reset(new World(comm_group));
		}
	}
	universe.gop.fence();
	return all_worlds;
}

/// base class
template<typename resultT, typename dataT>
class macro_task_base {
public:
	virtual resultT run(World& world, const dataT& data) = 0;
	virtual ~macro_task_base() {};

	double priority=0.0;
	bool no_task=true;

};

//
//template<typename T>
//T copy_to_redistributed(World& local_world, const T& source) {
//	print("copying", typeid(source).name());
//	return source;
//}
//
//template<typename keyT, typename valueT>
//WorldContainer<keyT, valueT> copy_to_redistributed(World& local_world,
//		const WorldContainer<keyT, valueT>& source) {
//
//	WorldContainer<keyT, valueT> local(local_world);
//    for (auto it : source) {
//        const auto& key = it.first;
//        const auto& value = it.second;
//        local.replace(key,value);
//    }
//    source.get_world().gop.fence();
//
//	return local;
//}
//

template<typename T, std::size_t NDIM>
struct data_type {
	typedef Function<T,NDIM> functionT;
	data_type() : i(), d(), f() {}
	data_type(const int& i, const double& d, Function<T,NDIM> f) : i(i), d(d), f(f),
			filename("dummy"+std::to_string(i)) {}
	data_type(const int& i, const double& d) : i(i), d(d), f(),
			filename("dummy"+std::to_string(i)) {}
//	data1(World& world) : i(), d(), f(Function<T,NDIM>) {}
	double d;
	int i;
	Function<T,NDIM> f;
	std::string filename="dummy";

    template <typename Archive>
    void serialize(const Archive& ar) {
    	bool fexist=f.is_initialized();
        ar & i & d & filename;
//		if (fexist) ar & f;

    }
	void save(World& world) const {
		world.gop.fence();
		archive::ParallelOutputArchive ar(world, filename.c_str() , 1);
//		print("saving to file",filename);
		ar & d & i & f;
	}

	void load(World& world) {
		world.gop.fence();
        auto pmap = std::shared_ptr< WorldDCPmapInterface< Key<NDIM> > >(new madness::LevelPmap< Key<NDIM> >(world));
        FunctionDefaults<NDIM>::set_pmap(pmap);	// set default pmap to use only this world!
//		print("loading from file",filename, world.id());
		archive::ParallelInputArchive ar(world, filename.c_str() , 1);
		ar & d & i & f;
	}

};



template<typename resultT, typename dataT>
class macro_task : public macro_task_base<resultT, dataT> {
public:
	double naptime=0.2; // in seconds
//	T data;

	macro_task() {}
	macro_task(bool real_task) {
		this->no_task=false;
	}
//	macro_task(const T& d) : data(d) {
//		this->no_task=false;
//	}


	resultT run(World& local, const dataT& data) {
//		if (local.rank()==0) print("task sleeping, performed by world",local.id());
//		std::random_device rd;
//		std::mt19937 mt(rd());
//		std::uniform_real_distribution<double> dist(0.1, 1.0);
//		double nap=dist(mt);
//
//		print("sleeping for",nap, "function_value is",data.f({0,0,0}));
//		myusleep(nap*1.e6);
		auto f=data.f;
		auto f2=square(f);
		double val1=f({0,0,0});
		double val2=f2({0,0,0});
		if (local.rank()==0) print("function_value is",val1,val2);
		local.gop.fence();
		return f2;
//		macro_task subsequent_task;
//		return std::make_pair(subsequent_task,T(f2));
	}

	~macro_task() {}


    template <typename Archive>
    void serialize(const Archive& ar) {
        ar & this->no_task & this->priority;
    }
	void save(World& world, const dataT& data) const {
		data.save(world);
	}

	void load(World& world, dataT& data) {
		data.load(world);
	}

};



class MasterPmap : public WorldDCPmapInterface<long> {
public:
    MasterPmap() {}
    ProcessID owner(const long& key) const {return 0;}
};



template<typename taskT>
class macro_taskq : public WorldObject< macro_taskq<taskT> > {
    typedef macro_taskq<taskT> thistype; ///< Type of this class (implementation)
    typedef WorldContainer<long,taskT> dcT; ///< Type of this class (implementation)

    World& universe;
    std::shared_ptr<World> regional_ptr;
	WorldContainer<long,taskT> taskq;

public:

	World& get_regional() {return *regional_ptr;}

    /// create an empty taskq and initialize the regional world groups
	macro_taskq(World& universe, int nworld)
		  : universe(universe), WorldObject<thistype>(universe),
			taskq(universe,std::shared_ptr< WorldDCPmapInterface<long> > (new MasterPmap())) {

		regional_ptr=(create_worlds(universe,nworld));
	    World& regional=*(regional_ptr.get());
		this->process_pending();
		taskq.process_pending();
	}

	void run_all() {

		World& regional=*(regional_ptr.get());
		universe.gop.fence();


		bool working=true;
		while (working) {
			std::shared_ptr<taskT> task=get_task_from_tasklist(regional);

			regional.gop.fence();
			if (task.get()) {
				task->load(regional);
				run_task(regional, *task);
			} else {
				working=false;
			}
		}
		universe.gop.fence();
	}

	void add_task(const std::pair<taskT,dataT>& task) {
		static int i=0;
		i++;
		int key=i;
		task.save(universe);
		if (universe.rank()==0) taskq.replace(key,task);
	};

	std::shared_ptr<taskT> get_task_from_tasklist(World& regional) {

		// only root may pop from the task list
		taskT task;
		if (regional.rank()==0) task=pop();
		regional.gop.broadcast_serializable(task, 0);

        if (task.no_task) return std::shared_ptr<taskT>();
		return std::shared_ptr<taskT>(new taskT(task));	// wait for task to arrive
	}

	void run_task(World& regional, taskT& task) {
		task.run(regional);
	};

	bool master() const {return universe.rank()==0;}

	std::size_t size() const {
		return taskq.size();
	}

	Future<taskT> pop() {
        return this->task(ProcessID(0), &macro_taskq<taskT>::pop_local, this->get_world().rank());
	}

	taskT pop_local(const int requested) {

		taskT result;
		bool found=false;
		while (not found) {
			auto iter=taskq.begin();
			if (iter==taskq.end()) {
				print("taskq empty");
				break;	// taskq empty
			}

			long key=iter->first;

			typename dcT::accessor acc;
			if (taskq.find(acc,key)) {
				result=acc->second;
				taskq.erase(key);
				break;
			} else {
				print("could not find key",key, "continue searching");
			}
		}

		return result;
	}

};


int main(int argc, char** argv) {
//    madness::World& universe = madness::initialize(argc,argv);
    initialize(argc, argv);
    World universe(SafeMPI::COMM_WORLD);
    startup(universe,argc,argv);

    std::cout << "Hello from " << universe.rank() << std::endl;
    universe.gop.fence();
    int nworld=std::min(int(universe.size()),int(3));
    if (universe.rank()==0) print("creating nworld",nworld);

    long ntask=20;

    // task-based model
	{
    	// data_type holds input and output data
    	typedef data_type<double,3> dataT;
    	typedef macro_task<Function<double,3>, dataT> taskT;
    	macro_taskq<std::pair<taskT,dataT> > taskq(universe,nworld);

    	for (int i=0; i<ntask; ++i) {
			Function<double,3> f(universe);
			f.add_scalar(i);
			dataT d(i,i,f);
			taskq.add_task(std::make_pair(taskT(),d));						// runs on all processors
		}
		taskq.run_all(fence=true);


    	for (int i=0; i<ntask; ++i) {						// post-process result
			result[i]=task[i].d.result;
		}

    }


	// vectorization model
    {
    	std::vector<data_type<double,3> > vinput(ntask);	// fill with input data
    	macro_task<data_type<double,3> > task;				// implements run(World& world, const data_type& d);
    	macro_taskq<taskT> taskq(universe,nworld);
    	std::vector<Function<double,3> > result=taskq.run_all(task,vinput,fence=true);
    }

//	// set up tasks with input data
//	std::vector<Function<double,3> > vfunction;
//	for (int i=0; i<ntask; ++i) {
//		Function<double,3> f(universe);
//		f.add_scalar(i);
//		vfunction.push_back(f);
//	}



    madness::finalize();
    return 0;
}

template <> volatile std::list<detail::PendingMsg> WorldObject<macro_taskq<macro_task<data_type<double, 3ul> > > >::pending = std::list<detail::PendingMsg>();
template <> Spinlock WorldObject<macro_taskq<macro_task<data_type<double, 3ul> > > >::pending_mutex(0);

template <> volatile std::list<detail::PendingMsg> WorldObject<WorldContainerImpl<int, double, madness::Hash<int> > >::pending = std::list<detail::PendingMsg>();
template <> Spinlock WorldObject<WorldContainerImpl<int, double, madness::Hash<int> > >::pending_mutex(0);

template <> volatile std::list<detail::PendingMsg> WorldObject<WorldContainerImpl<long, macro_task<data_type<double, 3ul> >, madness::Hash<long> > >::pending = std::list<detail::PendingMsg>();
template <> Spinlock WorldObject<WorldContainerImpl<long, macro_task<data_type<double, 3ul> >, madness::Hash<long> > >::pending_mutex(0);