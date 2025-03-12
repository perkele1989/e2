
#include "e2/managers/assetmanager.hpp"

#include "e2/managers/asyncmanager.hpp"

#include "e2/log.hpp"
#include <filesystem>

e2::AssetManager::AssetManager(Engine* owner)
	: e2::Manager(owner)
	, m_database(this)
{
	m_aljStates.resize(255);
}

e2::AssetManager::~AssetManager()
{

}

bool e2::AssetManager::prescribeALJ(e2::ALJDescription& target, e2::Name name)
{
	e2::AssetEntry* entry = m_database.entryFromName(name);
	if (!entry)
	{
		LogError("Failed to prescribe asset, asset entry does not exist: {}", name);
		return false;
	}

	target.names.insert(entry->name);
	return true;
}

void e2::AssetManager::processQueue()
{
retry:
	// fetch a queue entry
	e2::ALJQueueEntry queueEntry;
	{
		std::scoped_lock lock(m_aljMutex);
		if (m_aljQueue.empty())
			return;

		queueEntry = m_aljQueue.front();
		m_aljQueue.pop();
	}

	m_aljIndex = queueEntry.index;
	e2::ALJStateInternal& workingState = m_aljStates[m_aljIndex];
	workingState.groups.reserve(3);

	// seed dependencies with the uuids we want, then process them recursively
	std::unordered_set<e2::Name> dependencies = queueEntry.description.names;
	while (!dependencies.empty())
	{
		std::unordered_set<e2::Name> newGroup;
		std::unordered_set<e2::Name> newDependencies;
		for (e2::Name name : dependencies)
		{
			e2::AssetEntry* entry = m_database.entryFromName(name);
			assert(entry);

			bool isLoadedAlready = m_nameIndex.contains(name);
			bool isDuplicate = newGroup.contains(name);
			if (!isLoadedAlready && !isDuplicate)
				newGroup.insert(name);

			// remove it from any sets before this, since any dependency needs to be furthest back in the chain
			for (std::unordered_set<e2::Name>& currentGroup : workingState.groups)
				currentGroup.erase(name);

			// add dependencies from these assets too
			for (e2::DependencySlot slot : entry->header.dependencies)
			{
				if (slot.assetName.index() == 0)
				{
					LogError("null dependency!");
				}
				newDependencies.insert(slot.assetName);
			}
		}

		// add to existing sets
		workingState.groups.push_back(newGroup);



		// swap dependencies
		dependencies.swap(newDependencies);
	}

	// if we got no groups, everything is loaded already and we can retry with the next in queue
	if (workingState.groups.empty())
	{
		std::scoped_lock lock(workingState.publicStateMutex);
		workingState.publicState.status = e2::ALJStatus::Completed;
		goto retry;
	}

	m_aljProcessing = true;

	// We can begin processing the new queue member directly to get a head start
	processAlj();
}

void e2::AssetManager::processAlj()
{
	e2::ALJStateInternal& workingState = m_aljStates[m_aljIndex];
	if (workingState.submitted)
	{
		for (e2::AsyncTaskPtr task : workingState.taskList)
		{
			e2::AsyncTaskStatus status = task->status();
			if (status == AsyncTaskStatus::Processing)
			{
				return;
			}

			if (status == AsyncTaskStatus::Failed)
			{
				workingState.failure = true;
			}
		}

		// if we get this far, all tasks in this group are done processing

		// Add the new assets to our working state
		for (e2::AsyncTaskPtr task : workingState.taskList)
		{
			e2::AssetTaskPtr assetTask = task.cast<e2::AssetTask>();
			workingState.assets.insert(assetTask->asset());
			//m_uuidIndex[assetTask->entry()->uuid] = assetTask->asset();
		}

		// Clear the tasklist and submission status
		workingState.taskList.clear();
		workingState.submitted = false;

		// If this is true, the ALJ is complete
		if (++workingState.activeGroupIndex >= workingState.groups.size())
		{
			std::scoped_lock lock(workingState.publicStateMutex);
			if(workingState.failure)
				workingState.publicState.status = ALJStatus::Failed;
			else 
				workingState.publicState.status = ALJStatus::Completed;

			//workingState.assets

			m_aljProcessing = false;
		}
	}
	else
	{
		// Since we want the order of this vector reversed, we can just correct the indices instead
		uint8_t correctedGroupIndex = (uint8_t)workingState.groups.size() - workingState.activeGroupIndex - 1;
		std::unordered_set<e2::Name>& workingGroup = workingState.groups[correctedGroupIndex];


		workingState.taskList.reserve(workingGroup.size());

		for (e2::Name name: workingGroup)
		{
			e2::AssetEntry* entry = m_database.entryFromName(name);
			e2::AssetTaskPtr newTask = e2::AssetTaskPtr::create(this, entry);
			workingState.taskList.push_back(newTask.cast<e2::AsyncTask>());
		}

		asyncManager()->enqueue(workingState.taskList);
		workingState.submitted = true;
	}
}


e2::ALJTicket e2::AssetManager::queueALJ(e2::ALJDescription const& description)
{
	e2::ALJQueueEntry newEntry;
	newEntry.description = description;

	std::scoped_lock lock(m_aljMutex);
	newEntry.index = m_aljStateIds.create();

	m_aljQueue.push(newEntry);

	e2::ALJTicket newTicket;
	newTicket.id = newEntry.index;
	m_aljStates[newTicket.id].clear();
	m_aljStates[newTicket.id].valid = true;
	m_aljStates[newTicket.id].publicState.status = e2::ALJStatus::Queued;

	return newTicket;
}

e2::ALJState e2::AssetManager::queryALJ(e2::ALJTicket const& ticket)
{
	std::scoped_lock lock(m_aljStates[ticket.id].publicStateMutex);
	return m_aljStates[ticket.id].publicState;
}

void e2::AssetManager::returnALJ(e2::ALJTicket const& ticket)
{
	std::scoped_lock lock(m_aljStates[ticket.id].publicStateMutex);

	if (m_aljStates[ticket.id].publicState.status == e2::ALJStatus::Processing)
	{
		LogError("MEMORY LEAK: Attempted to return ALJ ticket that was currently processing. You can only return an ALJ ticket that is done processing.");
	}

	m_aljStateIds.destroy(ticket.id);
	m_aljStates[ticket.id].clear();
}

bool e2::AssetManager::waitALJ(e2::ALJTicket const& ticket)
{
	while (true)
	{
		// Query the ALJ first, to prevent unneccessary overhead in case its already done 
		e2::ALJState state = queryALJ(ticket);
		if (state.status == ALJStatus::Completed || state.status == ALJStatus::Failed)
		{
			// Return the ticket 
			returnALJ(ticket);
			return state.status == ALJStatus::Completed;
		}

		// Yield the thread to give breathing room for the worker threads
		// (if we stress the workerthreads by having an unbound loop here, we will induce a LOT of CPU usage and thus higher latency and system heat)
		std::this_thread::yield();

		// If this is called from main thread, we need to process the async and asset managers inline, otherwise we will block indefinitely and freeze the engine
		// Do this after the yield since we want as up-to-date information as possible when we query to reduce latency
		if (asyncManager()->isMainthread())
		{
			asyncManager()->update(0.0);
			update(0.0);

		}
	}
}

bool e2::AssetManager::queueWaitALJ(e2::ALJDescription const& description)
{
	e2::ALJTicket ticket = queueALJ(description);
	return waitALJ(ticket);

}

void e2::AssetManager::initialize()
{

}

void e2::AssetManager::shutdown()
{
	m_nameIndex.clear();
}

void e2::AssetManager::preUpdate(double deltaTime)
{

}

void e2::AssetManager::update(double deltaTime)
{
	// if we're processing an ALJ, keep doing that
	if (m_aljProcessing)
	{
		processAlj();
	}
	else
	{ 
		processQueue();
	}
}

e2::AssetPtr e2::AssetManager::get(e2::Name name)
{
	auto finder = m_nameIndex.find(name);
	if (finder != m_nameIndex.end())
	{
		return e2::Ptr<e2::Asset>(finder->second);
	}

	LogError("Attempted to get asset that is not loaded: {}", name);
	return nullptr;
}

e2::AssetTask::AssetTask(e2::Context* context, e2::AssetEntry* entry)
	: e2::AsyncTask(context)
	, m_entry(entry)
{
	
}

e2::AssetTask::~AssetTask()
{
	
}


bool e2::AssetTask::prepare()
{
	m_timer.reset();
	e2::Type* type = e2::Type::fromName(m_entry->header.assetType);
	if (!type)
	{
		LogError("Attempted to load asset of unregistered type");
		return false;
	}

	static const e2::Name assetTypeName = "e2::Asset";
	if (!type->inherits(assetTypeName))
	{
		LogError("Attempted to load asset that does not inherit from e2::Asset");
		return false;
	}

	e2::Object* object = type->create();
	if (!object)
	{
		LogError("Attempted to load asset of incomplete type");
		return false;
	}

	m_asset = e2::Ptr<e2::Asset>::emplace(object->cast<e2::Asset>());
	if (!m_asset)
	{
		LogError("Broken reflection");
		return false;
	}

	m_asset->postConstruct(this, m_entry->name);

	return true;
}

bool e2::AssetTask::execute()
{
	e2::FileStream fileStream(m_entry->path, e2::FileMode::ReadOnly);

	if (!fileStream.valid())
	{
		LogError("Asset file not readable or doesn't exist");
		return false;
	}

	e2::AssetHeader header;
	fileStream >> header;

	// Copy deps over to the asset instance, since this isn't read in the actual asset
	m_asset->dependencies = header.dependencies;
	m_asset->version = header.version;

	// @todo data compression

	if (!m_asset->read(fileStream))
	{
		LogError("Asset data corrupted");
		return false;
	}

	return true;
}

bool e2::AssetTask::finalize()
{
	bool returner = m_asset->finalize();
	if (returner)
	{
		double fullMs = m_timer.seconds() * 1000.0f;
		LogNotice("{}: {} {:4.1f}ms {:4.1f}ms", m_threadName, m_entry->name, m_asyncTime, fullMs );
		assetManager()->m_nameIndex[m_entry->name] = m_asset;
		return true;
	}
	return false;

}

e2::AssetEntry* e2::AssetTask::entry()
{
	return m_entry;
}

e2::AssetPtr e2::AssetTask::asset()
{
	return m_asset;
}

e2::AssetDatabase::AssetDatabase(e2::Context* ctx)
	: m_engine(ctx->engine())
{
	repopulate();
	dumpAssets();

	//populateEditorEntries();
}

e2::AssetDatabase::~AssetDatabase()
{
	clear();
}

void e2::AssetDatabase::clear()
{
	for (e2::AssetEntry* entry : m_assets)
	{
		e2::destroy(entry);
	}
	m_assets.clear();
	m_pathIndex.clear();
	m_nameIndex.clear();
	clearEditorEntries();
	
}

void e2::AssetDatabase::repopulate()
{
	for (e2::AssetEntry* entry : m_assets)
	{
		e2::destroy(entry);
	}
	m_assets.clear();

	// regenerate registry  
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("./assets/"))
	{
		if (!entry.is_regular_file())
			continue;

		if (entry.path().extension() != ".e2a")
			continue;

		invalidateAsset(entry.path().string());
	}

	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("./engine/"))
	{
		if (!entry.is_regular_file())
			continue;

		if (entry.path().extension() != ".e2a")
			continue;

		invalidateAsset(entry.path().string());
	}
}


e2::AssetEntry* e2::AssetDatabase::entryFromName(e2::Name name)
{
	auto finder = m_nameIndex.find(name);
	if (finder == m_nameIndex.end())
	{
		LogError("Failed to find asset given the name \"{}\"",name);
		return nullptr;
	}

	return finder->second;
}

e2::Engine* e2::AssetDatabase::engine()
{
	return m_engine;
}

e2::Name e2::AssetDatabase::invalidateAsset(std::string const& path)
{
	e2::AssetEntry* existing = entryFromPath(path);
	if(existing)
	{
		return existing->name;
	}

	std::string clean = cleanPath(path);

	e2::AssetEntry *newEntry = e2::create<e2::AssetEntry>();

	newEntry->path = clean;
	newEntry->name = std::filesystem::path(newEntry->path).filename().string();

	if (!readHeader(clean, newEntry->header))
	{
		LogError("Asset header couldnt be read on disk: \"{}\"", path);
		e2::destroy(newEntry);
	}

	m_assets.insert(newEntry);
	m_pathIndex[newEntry->path] = newEntry;
	m_nameIndex[newEntry->name] = newEntry;

	//populateEditorEntries();

	return newEntry->name;
}

bool e2::AssetDatabase::readHeader(std::string const& path, e2::AssetHeader& outHeader)
{
	std::string clean = cleanPath(path);

	e2::FileStream dataBuffer(clean, e2::FileMode::ReadOnly);
	if (!dataBuffer.valid())
	{
		return false;
	}

	return outHeader.read(dataBuffer);
}

std::string e2::AssetDatabase::cleanPath(std::string const& path)
{
	std::string b = std::filesystem::relative(std::filesystem::path(path), std::filesystem::current_path()).string();
	b = e2::replace("\\", "/", b);
	return b;
}


namespace
{

	void populateEntries(e2::AssetDatabase* db, std::shared_ptr<e2::AssetEditorEntry> editorEntry, std::string const& fullPath)
	{
		
		for (std::filesystem::directory_entry const& directoryEntry : std::filesystem::directory_iterator{ fullPath })
		{
			
			if (directoryEntry.is_directory())
			{
				std::shared_ptr<e2::AssetEditorEntry> newEntry = std::make_shared<e2::AssetEditorEntry>();
				newEntry->name = std::format("{}/", directoryEntry.path().filename().string());
				newEntry->parent = editorEntry;
				
				::populateEntries(db, newEntry, directoryEntry.path().string());

				editorEntry->children.push_back(newEntry);
			}
			else
			{
				std::filesystem::path assetPath = std::filesystem::relative(directoryEntry.path(), std::filesystem::absolute("./"));

				e2::AssetEntry* assetEntry = db->entryFromPath(assetPath.string());
				if (!assetEntry)
				{
					continue;
				}

				std::shared_ptr<e2::AssetEditorEntry> newEntry = std::make_shared<e2::AssetEditorEntry>();
				newEntry->name = assetPath.filename().string();
				newEntry->parent = editorEntry;
				newEntry->entry = assetEntry;
				
				editorEntry->children.push_back(newEntry);

				//LogNotice("Found editor asset: {}, of type {}: {}", assetEntry->path, assetEntry->header.assetType.cstring(), newEntry->fullPath().c_str());
			}
		}
	}
}

void e2::AssetDatabase::clearEditorEntries()
{

	if (m_rootEditorEntry)
	{
		struct _Unit
		{
			std::shared_ptr<e2::AssetEditorEntry> entry{};
		};

		std::queue<_Unit> q;
		q.push({ m_rootEditorEntry });

		while (!q.empty())
		{
			_Unit u = q.front();
			q.pop();

			for (std::shared_ptr<e2::AssetEditorEntry> c : u.entry->children)
			{
				q.push({ c });
			}
		}

		m_rootEditorEntry = nullptr;
	}
}

void e2::AssetDatabase::populateEditorEntries()
{
	clearEditorEntries();
	m_rootEditorEntry = std::make_shared<e2::AssetEditorEntry>();
	m_rootEditorEntry->name = "/";

	std::shared_ptr<e2::AssetEditorEntry> engineEntry = std::make_shared<e2::AssetEditorEntry>();
	engineEntry->name = "engine/";
	engineEntry->parent = m_rootEditorEntry;
	::populateEntries(this, engineEntry, "./engine/");
	m_rootEditorEntry->children.push_back(engineEntry);

	std::shared_ptr<e2::AssetEditorEntry>  assetsEntry = std::make_shared<e2::AssetEditorEntry>();
	assetsEntry->name = "assets/";
	assetsEntry->parent = m_rootEditorEntry;
	::populateEntries(this, assetsEntry, "./assets/");
	m_rootEditorEntry->children.push_back(assetsEntry);

}

std::shared_ptr<e2::AssetEditorEntry> e2::AssetDatabase::rootEditorEntry()
{
	return m_rootEditorEntry;
}

void e2::AssetDatabase::dumpAssets()
{
	LogNotice("dumping assets..:");
	for (e2::AssetEntry* entry : m_assets)
	{
		LogNotice("{}: {}: {}", entry->name, entry->header.assetType.cstring(), entry->path);
	}
}

e2::AssetEntry* e2::AssetDatabase::entryFromPath(std::string const& path)
{
	std::string clean = cleanPath(path);
	auto finder = m_pathIndex.find(clean);
	if (finder == m_pathIndex.end())
	{
		return nullptr;
	}

	return finder->second;
}

e2::AssetEntry::AssetEntry()
{

}

void e2::AssetEntry::write(e2::IStream& destination) const
{
	destination << name;
	destination << path;
	destination << timestamp;
	destination << header;
}

bool e2::AssetEntry::read(e2::IStream& source)
{
	source >> name;
	source >> path;
	source >> timestamp;
	source >> header;

	return true;
}


void e2::AssetHeader::write(e2::IStream& destination) const
{
	destination << e2::AssetMagic;
	destination << assetType;
	destination << uint64_t(version);
	destination << size;
	destination << uint8_t(dependencies.size());
	for (e2::DependencySlot const& depSlot : dependencies)
	{
		destination << depSlot.dependencyName;
		destination << depSlot.assetName;
	}
}

bool e2::AssetHeader::read(e2::IStream& source)
{
	uint64_t magic{};
	source >> magic;

	if (magic != e2::AssetMagic)
	{
		LogError("%s", "Invalid asset: Magic mismatch");
		return false;
	}

	e2::Name readType{};
	source >> readType;

	e2::Type* type = e2::Type::fromName(readType);

	if (!type || !type->inherits("e2::Asset"))
	{
		LogError("Invalid asset: Type not supported");
		return false;
	}

	uint64_t readVersion{};
	source >> readVersion;

	if (version >= e2::AssetVersion::End)
	{
		LogError("%s", "Invalid asset: Version not supported");
		return false;
	}


	assetType = readType;
	version = e2::AssetVersion(readVersion);

	source >> size;

	dependencies.resize(0);
	uint8_t numDeps{};
	source >> numDeps;
	for (uint8_t i = 0; i < numDeps; i++)
	{
		e2::DependencySlot depSlot;
		source >> depSlot.dependencyName;
		source >> depSlot.assetName;
		dependencies.push(depSlot);
	}

	return true;
}

void e2::ALJStateInternal::clear()
{
	valid = false;
	publicState = ALJState();
	groups.clear();
	activeGroupIndex = 0;
	assets.clear();
	taskList.clear();
	submitted = false;
	failure = false;
}


std::string e2::AssetEditorEntry::fullPath()
{
	if (auto p = parent.lock())
	{
		return std::format("{}{}", p->fullPath(), name);
	}

	return name;

}
