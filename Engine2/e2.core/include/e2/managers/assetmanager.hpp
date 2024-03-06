
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>
#include <e2/assets/asset.hpp>

#include <e2/async.hpp>
#include <e2/timer.hpp>

#include <unordered_map>
#include <unordered_set>
#include <queue>

namespace e2
{   
	constexpr uint64_t AssetMagic = 0x0000E2A55EF00000;

	class AssetEntry;

	struct E2_API AssetHeader : public e2::Data
	{

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		e2::Name assetType;
		e2::AssetVersion version{ e2::AssetVersion::Latest };
		uint64_t size{};
		e2::StackVector<e2::DependencySlot, e2::maxNumAssetDependencies> dependencies;
	};

	/** @tags(arena, arenaSize=1024) */
	class E2_API AssetTask : public e2::AsyncTask
	{
		ObjectDeclaration()
	public:
		AssetTask(e2::Context* context, e2::AssetEntry* entry);
		virtual ~AssetTask();

		virtual bool prepare()override;
		virtual bool execute() override;
		virtual bool finalize() override;

		e2::AssetEntry* entry();
		e2::AssetPtr asset();

	protected:
		e2::Timer m_timer;
		e2::AssetEntry* m_entry{};
		e2::AssetPtr m_asset{};
		
	};



	/** @tags(arena, arenaSize=e2::maxNumAssetEntries) */
	class E2_API AssetEntry : public e2::Data, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		AssetEntry();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		e2::UUID uuid;
		uint64_t timestamp{};
		std::string path;
		e2::AssetHeader header;
	};

	// editor only utility, but still likely want to optimize data storage for this one (inline list iterators?)
	struct E2_API AssetEditorEntry
	{
		std::string fullPath();

		inline bool isFolder() const
		{
			return name.ends_with("/");
		}

		AssetEditorEntry* parent{ };
		std::string name; // folders end with / 
		std::list<AssetEditorEntry*> children;

		/** The entry for this one, if it's an asset and not a folder*/
		AssetEntry* entry{};
	};


	/** Represents the asset database. Keeps track of all assets in a project and its metadata. It does NOT keep track of loaded assets nor loads them.*/
	class E2_API AssetDatabase : public e2::Context, public e2::Data
	{
	public:
		AssetDatabase(e2::Context* ctx);
		virtual ~AssetDatabase();

		void readFromDisk();
		void writeToDisk();

		void validate(bool forceSave);

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		AssetEntry *entryFromPath(std::string const &path);
		AssetEntry* entryFromUUID(e2::UUID const &uuid);

		virtual Engine* engine() override;
		e2::UUID invalidateAsset(std::string const& path);

		static bool readHeader(std::string const& path, e2::AssetHeader& outHeader);
		static std::string cleanPath(std::string const& path);

		void populateEditorEntries();
		e2::AssetEditorEntry* rootEditorEntry();

	protected:

		e2::Engine* m_engine{};
		std::unordered_set<e2::AssetEntry*> m_assets;

		std::unordered_map<std::string, e2::AssetEntry*> m_pathIndex;
		std::unordered_map<e2::UUID, e2::AssetEntry*> m_uuidIndex;

		/** nullptr in game builds */
		e2::AssetEditorEntry* m_rootEditorEntry{};
	};

	/** Asset-Load-Job description */
	struct E2_API ALJDescription
	{
		/** Load these uuids (no need to include deps, they are figured out and loaded automatically) */
		std::unordered_set<e2::UUID> uuids;
	};

	/**
	 * An asset pool, can be used to pin assets in memory for as long as this object exists
	 * @tags(arena, arenaSize=e2::maxNumAssetPools)
	 */
	class E2_API AssetPool : public e2::ManagedObject
	{
		ObjectDeclaration()
	public:

		virtual ~AssetPool();

		std::unordered_set<e2::AssetPtr> assets;
	};

	/** Asset-Load-Job ticket */
	struct E2_API ALJTicket
	{
		uint8_t id{};
	};

	enum class ALJStatus : uint8_t
	{
		Unused = 0,
		Queued,
		Processing,
		Completed,
		Failed
	};

	struct E2_API ALJState
	{
		e2::ALJStatus status { e2::ALJStatus::Unused };
	};

	struct E2_API ALJStateInternal
	{
		void clear();

		bool valid{ false };

		ALJState publicState;
		std::mutex publicStateMutex;
		std::vector<std::unordered_set<e2::UUID>> groups;
		uint32_t activeGroupIndex{};

		bool failure{};
		bool submitted{};
		std::vector<e2::AsyncTaskPtr> taskList;

		std::unordered_set<e2::AssetPtr> assets;

	};

	struct E2_API ALJQueueEntry
	{
		ALJDescription description;
		uint8_t index{};
	};

	/** The actual asset manager. Stores an asset database full of metadata, and additionally loads and tracks loaded assets. */
	class E2_API AssetManager : public e2::Manager
	{
	public:

		AssetManager(e2::Engine* owner);
		virtual ~AssetManager();

		/** Prescribe the given asset to the given ALJ (asset load job) description. Returns true if successful, will not touch description if failed. */
		bool prescribeALJ(e2::ALJDescription& target, std::string const& assetPath);

		/** Given a asset load job description, queues the job and returns a ticket. Will generate a dependency tree to maximize asynchronicity. Don't forget to return the ticket back when done! */
		e2::ALJTicket queueALJ(e2::ALJDescription const& description);

		/** Query the asset load job state. Will be able to do this for as long as we don't return the ticket with returnTicket. */
		e2::ALJState queryALJ(e2::ALJTicket const& ticket);

		/** Returns the ticket back to the system. Frees resources for this job, but not the assets. Invalid to do this for a state that is being processed */
		void returnALJ(e2::ALJTicket const& ticket);

		/** Blocking wait for the given ticket. Returns true if assets loaded properly. Note: This will invoke returnALJ before returning! */
		bool waitALJ(e2::ALJTicket const& ticket);

		/** Queues an ALJ and waits for it. Returns true if assets loaded properly . */
		bool queueWaitALJ(e2::ALJDescription const& description);

		/** Create an asset pool from an ALJDescription. Protip: Reuse the one from a loadjob to pin the loaded assets in place after you've returned the ticket. */
		e2::AssetPool* createAssetPool(ALJDescription const& description);


		virtual void initialize() override;
		virtual void shutdown() override;

		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

		inline AssetDatabase &database()
		{
			return m_database;
		}

		/** retrieves a loaded asset, or nullptr with an error if it doesnt exist */
		e2::AssetPtr get(e2::UUID const& uuid);
		e2::AssetPtr get(std::string const& assetPath);

	protected:

		friend e2::AssetTask;

		void processQueue();
		void processAlj();

		AssetDatabase m_database;

		/** Loaded assets, indexed by uuid */
		std::unordered_map<e2::UUID, e2::AssetPtr> m_uuidIndex;

		std::queue<e2::ALJQueueEntry> m_aljQueue;
		std::mutex m_aljMutex;
		bool m_aljProcessing{};
		uint8_t m_aljIndex{};
		e2::IdArena<uint8_t, 255> m_aljStateIds;
		e2::StackVector<ALJStateInternal, 255> m_aljStates;
	
	};
	
}

#include "assetmanager.generated.hpp"