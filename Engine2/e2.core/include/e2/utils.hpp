
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/buffer.hpp>

#include <glm/glm.hpp>

#include <functional>
#include <vector>
#include <string>
#include <array>
#include <memory>
#include <set>
#include <any>
#include <atomic>


#define EnumFlagsDeclaration(x) constexpr x operator|(x lhs, x rhs) \
{ \
    return static_cast<x>(static_cast<std::underlying_type<x>::type>(lhs) | static_cast<std::underlying_type<x>::type>(rhs)); \
} \
constexpr x & operator|=(x & lhs, x const& rhs) \
{ \
	lhs = lhs | rhs; \
    return lhs; \
} \
constexpr x operator&(x lhs, x rhs) \
{ \
    return static_cast<x>(static_cast<std::underlying_type<x>::type>(lhs) & static_cast<std::underlying_type<x>::type>(rhs)); \
} \
constexpr x operator^(x lhs, x rhs) \
{ \
    return static_cast<x>(static_cast<std::underlying_type<x>::type>(lhs) ^ static_cast<std::underlying_type<x>::type>(rhs)); \
}

// Macro for reflected types (i.e. classes that inherit from e2::Object)
//#if defined(E2_SCG)
//#define E2_CLASS_DECLARATION() 
//#else
#define ObjectDeclaration() \
public:\
	static e2::Type const* staticType();\
	virtual e2::Type const* type() override;\
	static void registerType();
//#endif


namespace e2
{
	template<typename T>
	class E2_API Pair
	{
	public:

		Pair()
		{

		}

		Pair(T const& initializer)
		{
			items[0] = initializer;
			items[1] = initializer;
		}

		T& operator[](uint8_t index)
		{
			return items[index];
		}

		T* data()
		{
			return &items[0];
		}

	private:
		T items[2];
	};

	/**
	 * Utility for doublebuffered parameters (perfect for source data for descriptor sets)
	 */
	template<typename DataType>
	class E2_API DirtyParameter
	{
	public:

		/** Sets the source data and marks dirty for both frames */
		void set(DataType const& newData)
		{
			m_data = newData;
			m_dirty = { true };
		}

		/** Checks if the given frame is dirty, and clears the dirty flag for the given frame if it is*/
		bool invalidate(uint8_t frameIndex)
		{
			bool dirty = m_dirty[frameIndex];
			if(dirty)
				m_dirty[frameIndex] = false;

			return dirty;
		}

		DataType const& data()
		{
			return m_data;
		}

	protected:
		DataType m_data{};
		e2::Pair<bool> m_dirty {true};
	};

	struct E2_API UUID : public e2::Data
	{
		static UUID generate();
		UUID();
		UUID(std::string const& str);
		std::string string() const;

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		bool valid();

		std::array<uint8_t, 16> data{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	};



	inline bool operator== (UUID const& lhs, UUID const& rhs) noexcept
	{
		return lhs.data == rhs.data;
	}

	inline bool operator!= (UUID const& lhs, UUID const& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	inline bool operator< (UUID const& lhs, UUID const& rhs) noexcept
	{
		return lhs.data < rhs.data;
	}

	E2_API glm::vec2 rotate2d(glm::vec2 const& vec, float angleDegrees);

	E2_API float radiansBetween(glm::vec3 const& a, glm::vec3 const & b);;

	E2_API size_t hash(e2::UUID const& id);

	template <class T>
	inline void hash_combine(size_t& seed, const T& v)
	{
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
	}

	E2_API bool nearlyEqual(float x, float y, float tresh);
	E2_API bool intersect(glm::vec2 p, glm::vec2 boxOffset, glm::vec2 boxSize);

	E2_API bool randomBool();
	E2_API int64_t randomInt(int64_t min, int64_t max);
	E2_API float randomFloat(float min, float max);
	E2_API double randomDouble(double min, double max);
	E2_API glm::ivec2 randomIvec2(glm::ivec2 min, glm::ivec2 max);
	E2_API glm::vec2 randomVec2(glm::vec2 min, glm::vec2 max);
	E2_API glm::vec3 randomVec3(glm::vec3 min, glm::vec3 max);
	E2_API glm::vec2 randomOnUnitCircle();
	E2_API glm::vec2 randomInUnitCircle();
	E2_API glm::vec3 randomOnUnitSphere();
	E2_API glm::vec3 randomInUnitSphere();



	E2_API std::string replace(std::string const& needle, std::string const& replacement, std::string const& haystack);

	E2_API std::string trim(std::string const& input);
	E2_API std::string trim(std::string const& input, std::vector<char> const& characters);
	E2_API std::vector<std::string> split(std::string const& input, char delimiter);


	E2_API std::string toLower(std::string const& input);

	E2_API std::string utf32to8(std::u32string const& utf32);
	E2_API std::u32string utf8to32(std::string const& utf8);
	E2_API std::string genericToUtf8(std::string const& generic);

	E2_API bool readFile(std::string const& path, std::string& outData);
	E2_API bool writeFile(std::string const& path, std::string const& data);

	/**
	 * Reads a string file, but allows c++/glsl-style includes to be made.
	 * All paths are relative to the current working directory
	 */
	E2_API bool readFileWithIncludes(std::string const& path, std::string& resolvedData);


	E2_API uint32_t calculateMipLevels(glm::uvec2 const& resolution);

	template <uint64_t Capacity>
	concept Empty = Capacity == 0ULL;

	/** 
	 * Dynamic vector with static allocation 
	 * --
	 * The entire capacity of the data is implicitly stored where the vector itself is stored.
	 * If the vector is stored on the stack, so is the data.
	 * If the vector is stored on the heap, the data is implicitly stored in the same allocation.
	 * No allocations are made.
	 * Data is stored linearly.
	 * 
	 * Time complexities:
	 * O(1) for push (inserting at end of vector)
	 * O(1) for pop (removing at end of vector)
	 * O(1) for clear (removing all elements)
	 * O(1) for resize
	 * O(1) for removeByIndexFast (does not preserve order)
	 * O(n-i) for removeByIndexOrdered (preserves order)
	 * O(n) for iteration
	 * O(n) at worst for contains/find, but usually faster
	 * 
	 * removeByValueFast / removeByValueOrdered are simply find followed by their respective removeByIndex* versions 
	 * removeAllByValueOrdered is pretty slow but in comparison but optimized for the purpose.
	 */
	template<typename DataType, uint64_t Capacity>
		requires (!Empty<Capacity>)
	class StackVector
	{
	public:

		struct Iterator
		{
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = DataType;
			using pointer = DataType*;
			using reference = DataType&;

			friend bool operator== (const Iterator& a, const Iterator& b)
			{
				return a.m_ptr == b.m_ptr;
			}

			friend bool operator!= (const Iterator& a, const Iterator& b)
			{
				return a.m_ptr != b.m_ptr;
			}




			Iterator(pointer ptr)
				: m_ptr(ptr)
			{
			}

			Iterator& operator++()
			{
				m_ptr++; return *this;
			}

			Iterator operator++(int)
			{
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			reference operator*() const
			{
				return *m_ptr;
			}

			pointer operator->()
			{
				return m_ptr;
			}

		protected:
			pointer m_ptr{};
		};


		struct ConstIterator
		{
			using iterator_category = std::forward_iterator_tag;
			using difference_type = std::ptrdiff_t;
			using value_type = DataType const;
			using pointer = DataType const*;
			using reference = DataType const&;

			friend bool operator== (const ConstIterator& a, const ConstIterator& b)
			{
				return a.m_ptr == b.m_ptr;
			}

			friend bool operator!= (const ConstIterator& a, const ConstIterator& b)
			{
				return a.m_ptr != b.m_ptr;
			}

			ConstIterator(pointer ptr)
			: m_ptr(ptr)
			{
			}

			ConstIterator& operator++()
			{
				m_ptr++; return *this;
			}

			ConstIterator operator++(int)
			{
				Iterator tmp = *this;
				++(*this);
				return tmp;
			}

			reference operator*() const
			{
				return *m_ptr;
			}

			pointer operator->()
			{
				return m_ptr;
			}

		protected:
			pointer m_ptr{};
		};

		StackVector()
		{
		}

		/** Fill-constructor, fills the stack vector with the given element*/
		StackVector(DataType const& fillSource)
		{
			// should do memset or similar here
			for (uint32_t i = 0; i < Capacity; i++)
			{
				push(fillSource);
			}
		}

		StackVector(std::initializer_list<DataType> initializer)
		{
			assert(initializer.size() <= Capacity);

			for (DataType const& item : initializer)
			{
				push(item);
			}
		}

		/** Adds a new unique element to this vector. Returns true if this vector contains the element when returning. */
		bool addUnique(DataType const& newElement)
		{
			if (contains(newElement))
				return true;

			return push(newElement);
		}

		/** Adds a new unique element to this vector. Returns true if this vector contains the element when returning. */
		bool addUnique(DataType&& newElement)
		{
			if (contains(newElement))
				return true;

			return push(newElement);
		}

		/** Removes the value by index and preserves order */
		void removeByIndexOrdered(uint32_t index)
		{
			if (index >= size())
				return;

			if (size() == 0)
				return;

			// last element optimization, just resize the vector
			if (index == size() - 1)
			{
				resize(size() - 1);
				return;
			}

			for (uint32_t i = index + 1; i < size(); i++)
			{
				m_data[i - 1] = m_data[i];
			}

			resize(size() - 1);
		}

		/** Removes the value by index, does not preserve order of elements */
		void removeByIndexFast(uint32_t index)
		{
			if (index >= size())
				return;

			if (size() == 0)
				return;

			// copy the last element into the index we want to remove, unless it is last, then resize the vector
			if (index != size() - 1)
				m_data[index] = m_data[size() - 1];

			resize(size() - 1);
		}

		/** removes all occurences of value from this stackvector, preserves order */
		void removeAllByValueOrdered(DataType const& value)
		{
			if (size() == 0)
				return;

			uint32_t copyBack = 0;
			for (uint32_t i = 0; i < size(); i++)
				if (m_data[i] == value)
					copyBack++;
				else if (copyBack > 0)
					m_data[i - copyBack] = m_data[i];

			if (copyBack > 0)
				resize(size() - copyBack);
		}

		/** removes first occurence of value from this stackvector, does not preserve order */
		void removeFirstByValueFast(DataType const& value)
		{
			int32_t index = find(value, 0);
			removeByIndexFast(uint32_t(index));
		}

		/** removes first occurence of value from this stackvector,  preserves order */
		void removeFirstByValueOrdered(DataType const& value)
		{
			int32_t index = find(value, 0);
			removeByIndexOrdered(uint32_t(index));
		}

		/** Finds the index of the first entry in the vector, starting at offset. Returns -1 if not found*/
		int32_t find(DataType const& comparison, uint32_t offset = 0)
		{
			for (int32_t i = int32_t(offset); i < int32_t(size()); i++)
			{
				if (m_data[i] == comparison)
					return i;
			}

			return -1;
		}

		bool contains(DataType const& comparison)
		{
			return find(comparison, 0) >= 0;
		}

		ConstIterator begin() const
		{
			return &m_data[0];
		}

		ConstIterator end() const
		{
			return &m_data[m_size];
		}

		Iterator begin()
		{
			return &m_data[0];
		}

		Iterator end()
		{
			return &m_data[m_size];
		}

		DataType& front()
		{
			assert(!empty() && "out of range");
			
			return m_data[0];
		}

		DataType& back()
		{
			assert(!empty() && "out of range");

			return m_data[m_size - 1];
		}

		void clear()
		{
			m_size = 0;
		}

		bool empty()
		{
			return m_size == 0;
		}

		bool push(DataType const &newData)
		{
			if (m_size >= Capacity)
			{
				LogError("out of bounds");
				return false;
			}

			m_data[m_size] = newData;

			m_size++;

			return true;
		}


		void push(DataType&& newData)
		{
			assert(m_size < Capacity);

			m_data[m_size] = newData;

			m_size++;

			return;
		}

		DataType& emplace()
		{
			assert(m_size < Capacity);

			m_size++;
			return m_data[m_size - 1];
		}

		bool pop() 
		{
			if (empty())
				return false;

			m_size--;

			return true;
		}

		DataType& at(size_t index)
		{
			assert((index < Capacity) && "out of bounds");

			// soft bounds check, will not crash but we still want to notify user
			if (index >= m_size)
			{
				LogError("out of bounds");
			}

			return m_data[index];
		}

		inline uint64_t size() const
		{
			return m_size;
		}

		DataType const* data() const
		{
			return m_data;
		}

		inline DataType* data()
		{
			return m_data;
		}

		DataType& operator[](size_t index)
		{
			assert((index < Capacity) && "out of bounds");

			// soft bounds check, will not crash but we still want to notify user
			if (index >= m_size)
			{
				LogError("out of bounds");
			}

			return m_data[index];
		}

		DataType const& operator[](size_t index) const
		{
			assert((index < Capacity) && "out of bounds");

			// soft bounds check, will not crash but we still want to notify user
			if (index >= m_size)
			{
				LogError("out of bounds");
			}

			return m_data[index];
		}

		void resize(uint64_t newSize)
		{
			if (newSize > Capacity)
			{
				LogWarning("new size out of range, capping to capacity {}/{}", newSize, Capacity);
				newSize = Capacity;
			}

			m_size = newSize;
		}

	protected:

		DataType m_data[Capacity];
		uint64_t m_size{};
	};


	/** 
	 * Cached string, use for commonly used shorter strings. Cached and reused. Overhead is 32bit unsigned integer.
	 * Serialized as a c-string, as the index is not guaranteed to be the same on every execution
	 */
	struct E2_API Name : public e2::Data
	{
	public:
		Name();
		Name(std::string const& s);
		Name(std::string_view v);
		Name(char const* c);

		~Name();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		inline uint32_t const index() const
		{
			return m_index;
		}

		std::string_view view() const;

		std::string string() const;

		char const* cstring() const;

		operator uint32_t& () { return m_index; }
		operator uint32_t () const { return m_index; }

		bool operator <(Name const& rhs) const
		{
			return m_index < rhs.m_index;
		}

	protected:
		uint32_t m_index{};
	};

	/** Single-allocation memory arena, with preallocated memory storage */
	template<typename DataType>
	class Arena
	{
		using AllocatorType = std::allocator<DataType>;
		using TraitsType = std::allocator_traits<AllocatorType>;

	public:

		Arena(uint64_t size)
			: m_size(size)
		{

			m_data = TraitsType::allocate(m_allocator, m_size);
			m_freeList = new uint64_t[m_size];
		}

		~Arena()
		{
			delete m_freeList;
			TraitsType::deallocate(m_allocator, m_data, m_size);
		}

		template<typename... Args>
		DataType* create(Args&&... args)
		{
			std::scoped_lock lock(m_mutex);
			uint64_t newId = 0;

			if (m_numFree > 0)
			{
				newId = m_freeList[m_numFree - 1];
				m_numFree--;
			}
			else
			{
				if (m_next >= m_size)
				{
					LogError("No more room in arena, returning nullptr");
					return nullptr;
				}

				newId = m_next++;
			}

			TraitsType::construct(m_allocator, &m_data[newId], std::forward<Args>(args)...);

			return &m_data[newId];
		}

		/** Creates the same as normal create, but returns an id instead. Returns UINT64_MAX on failure. */
		template<typename... Args>
		uint64_t create2(Args&&... args)
		{
			std::scoped_lock lock(m_mutex);
			uint64_t newId = 0;

			if (m_numFree > 0)
			{
				newId = m_freeList[m_numFree - 1];
				m_numFree--;
			}
			else
			{
				if (m_next >= m_size)
				{
					LogError("No more room in arena, returning UINT64_MAX");
					return UINT64_MAX;
				}

				newId = m_next++;
			}

			TraitsType::construct(m_allocator, &m_data[newId], std::forward<Args>(args)...);

			return newId;
		}

		/** Returns a raw pointer based on id from create2. O(1) */
		DataType* getById(uint64_t id)
		{
			std::scoped_lock lock(m_mutex);
			return &m_data[id];
		}

		void destroy(DataType* entry)
		{
			std::scoped_lock lock(m_mutex);
			// verify its within bounds 
			if (entry < m_data || entry >= m_data + sizeof(DataType) * m_size)
			{
				LogError("entry not within arena bounds!");
				return;
			}

			uint64_t entryId = (uintptr_t(entry) - uintptr_t(m_data)) / sizeof(DataType);
			TraitsType::destroy(m_allocator, &m_data[entryId]);

			m_numFree++;
			m_freeList[m_numFree - 1] = entryId;
		}

		void destroy2(uint64_t id)
		{
			std::scoped_lock lock(m_mutex);
			TraitsType::destroy(m_allocator, &m_data[id]);

			m_numFree++;
			m_freeList[m_numFree - 1] = id;
		}

	protected:
		std::mutex m_mutex;

		AllocatorType m_allocator;
		DataType* m_data{};
		uint64_t m_size{};
		uint64_t m_next{};

		uint64_t* m_freeList{};
		uint64_t m_numFree{};
	};




	/** Single-allocation memory arena for storing ids */
	template<typename IdType, IdType Size>
	class IdArena
	{
	public:

		IdArena()
		{

			m_freeList = new IdType[Size];
		}

		~IdArena()
		{
			delete m_freeList;
		}

		IdType create()
		{
			IdType newId = 0;

			if (m_numFree > 0)
			{
				newId = m_freeList[m_numFree - 1];
				m_numFree--;
			}
			else
			{
				if (m_next >= Size)
				{
					LogError("No more room in arena, returning max");
					return std::numeric_limits<IdType>::max();
				}

				newId = m_next++;
			}

			return newId;
		}

		void destroy(IdType id)
		{
			if (id >= Size)
			{
				LogError("entry not within arena bounds");
				return;
			}

			m_numFree++;
			m_freeList[m_numFree - 1] = id;
		}

	protected:
		IdType m_numFree{};
		IdType m_next{};
		IdType* m_freeList{};
	};


	class ManagedObject;
	class Object;
	

	/** A block that tracks managed objects */
	class E2_API ManagedBlock
	{
	public:

		e2::ManagedObject* object();

		void objIncrement();

		void objDecrement();

		void blockIncrement(); 

		void blockDecrement();

		bool isPendingKill();

		void flagPendingKill();

		std::atomic_bool pendingKill{};

		// How many references to object (for e2::Ptr<T> etc.)
		std::atomic_int64_t objRef{};

		// How many references to block (for e2::WeakPtr<T> etc.)
		std::atomic_int64_t blockRef{};

		// Pointer to actual data
		std::atomic_uintptr_t objPtr{};
	};

	E2_API e2::ManagedBlock* createBlock();
	E2_API void destroyBlock(e2::ManagedBlock* block);
	E2_API e2::ManagedObject* resolveBlock(e2::ManagedBlock* block);


	/** Describes the reflected type of a managed object */
	struct E2_API Type
	{
		/**
		* warning: only to be called by generated code, or under _very_ specific circumstances!
		* If you want to register custom types without using the reflection system (the proper way),
		* fill in the fields and then call this, however make absolutely sure that you never
		* deallocate or change the Type instance after you do this. Prefer to store the
		* Type as a static variable, or as a pure heap-allocated variable that you then don't touch.
		*/
		void registerType();

		static Type* fromName(e2::Name name);

		/** Allocates memory for, and constructs the object using the empty constructor, possibly using an arena */
		using CreateFunc = e2::Object* (*)();

		/** Frees and destructs the instance, possibly using an arena */
		using DestroyFunc = void (*)(e2::Object*);

		/** The name of this type, e.g. "Foo" */
		e2::Name name;

		bool isAbstract{};

		/** The fully qualified name of this type, e.g. "ns::Foo" */
		e2::Name fqn;

		/** this type directly inherits from these bases, in fqn @todo consider changing storage */
		std::set<e2::Name> bases;

		/** this type directly or indirectly inherits from these bases, in fqn */
		std::set<e2::Name> allBases;

		bool inherits(e2::Name baseType, bool includeAncestors = true);
		bool inheritsOrIs(e2::Name baseType, bool includeAncestors = true);

		std::set<e2::Type*> findChildTypes();

		CreateFunc create{};
		DestroyFunc destroy{};
	};

#if defined(E2_DEVELOPMENT)
	// Prints all managed objects, invoked before shutdown on dev builds to track managed memory leaks
	E2_API void printLingeringObjects();
#endif

	/**
	 * Baseclass for everything reflected
	 * Enforces vtable overhead, however no additional data members should be added to this!
	 * If you want to add more exotic functionality to a common baseclass, consider e2::ManagedObject
	 * When inheriting from Object, don't forget to add ObjectDeclaration() at the top of your class declaration block
	 */
	class E2_API Object
	{
	public:

		Object();
		virtual ~Object();

		static e2::Type const* staticType();
		static void registerType();

		virtual e2::Type const* type();

		template<typename ToType>
		ToType* cast()
		{
			return dynamic_cast<ToType*>(this);
		}

		template<typename ToType>
		ToType* unsafeCast()
		{
			return static_cast<ToType*>(this);
		}

	};

	/**
	 * Extends reflected e2::Object's with memory tracking capabilities
	 * to be used for things like e2::Ptr
	 * 
	 * If you don't need to support managed pointers, consider inheriting from e2::Object instead.
	 */
	class E2_API ManagedObject : public e2::Object
	{
		ObjectDeclaration()
	public:

		ManagedObject();
		virtual ~ManagedObject();

		static void keepAroundTick();
		static void keepAroundPrune();

		void keepAround();

		inline e2::ManagedBlock* block() const
		{
			return m_block;
		}

	private:
		e2::ManagedBlock* m_block{};
	};

	template <typename T>
	concept Reflected = std::is_base_of_v<e2::Object, T>;

	template <typename T>
	concept Managed = std::is_base_of_v<e2::ManagedObject, T>;

	template <typename T>
	class Allocator
	{
	public:
		static T* create(...) = delete;

		static void destroy(T*) = delete;
	};

	template <typename T>
		requires (!Reflected<T>)
	class Allocator<T>
	{
	public:
		template <typename ... Args>
		static T* create(Args&&... args)
		{
			return new T(std::forward<Args>(args)...);
		}

		static void destroy(T* instance)
		{
			delete instance;
		}
	};

	template<typename Type, typename ... Args>
	Type* create(Args&&... args)
	{
		return e2::Allocator<Type>::create(std::forward<Args>(args)...);
	}

	template<typename Type>
	void destroy(Type* instance)
	{
		return e2::Allocator<Type>::destroy(instance);
	} 

	template<typename Type>
	void discard(Type* instance)
	{
		instance->keepAround();
		e2::destroy(instance);
	}

	/** Stupid helper function. Dont know why we have it. We usually just use e2::Type::fromName(...) and create from that in most cases. */
	E2_API e2::Object* createDynamic(e2::Name typeName);

	template<typename Type>
		requires (Managed<Type>)
	class Ptr
	{
	public:

		template<typename DestinationType>
		e2::Ptr<DestinationType> cast() const
		{

			DestinationType* asDestination = dynamic_cast<DestinationType*>(m_object);

			e2::Ptr<DestinationType> newPtr(asDestination);

			return newPtr;
		}

		template<typename DestinationType>
		e2::Ptr<DestinationType> unsafeCast() const
		{

			DestinationType* asDestination = static_cast<DestinationType*>(m_object);

			e2::Ptr<DestinationType> newPtr(asDestination);

			return newPtr;
		}

		/** Creates the object directly, making this the owning pointer and thus acts like a RAII pointer */
		template<typename... Args>
		static Ptr<Type> create(Args&&... args)
		{
			e2::Ptr<Type> newPtr;
			newPtr.m_object = e2::create<Type>(std::forward<Args>(args)...);
			newPtr.m_object->block()->objIncrement();
			e2::destroy(newPtr.m_object);
			return newPtr;
		}

		/** Like Ptr::create, except lets caller specify an already created instance, making this the owning pointer and thus acts like a RAII pointer */
		template<typename... Args>
		static Ptr<Type> emplace(Type* instance)
		{
			e2::Ptr<Type> newPtr;
			newPtr.m_object = instance;
			newPtr.m_object->block()->objIncrement();
			e2::destroy(newPtr.m_object);
			return newPtr;
		}

		Type* get() const
		{
			return m_object;
		}

		constexpr Ptr() = default;

		constexpr Ptr(nullptr_t) noexcept {}

		~Ptr()
		{
			if (m_object)
				m_object->block()->objDecrement();

			m_object = nullptr;
		}

		Ptr(Type* object)
		{
			m_object = object;

			if (m_object)
				m_object->block()->objIncrement();
		}

		Ptr(const Ptr<Type>& rhs)
		{
			m_object = rhs.m_object;

			if (m_object)
				m_object->block()->objIncrement();
		}

		Ptr(Ptr&& rhs)
		{
			m_object = rhs.m_object;

			if (m_object)
				m_object->block()->objIncrement();
		}

		Ptr<Type>& operator=(Ptr<Type> const& rhs)
		{
			if (m_object == rhs.m_object)
				return *this;

			if (m_object)
				m_object->block()->objDecrement();

			m_object = rhs.m_object;

			if (m_object)
				m_object->block()->objIncrement();

			return *this;
		}

		Ptr<Type>& operator=(Ptr<Type>&& rhs)
		{
			if (m_object == rhs.m_object)
				return *this;

			if (m_object)
				m_object->block()->objDecrement();

			m_object = rhs.m_object;

			if (m_object)
				m_object->block()->objIncrement();

			return *this;
		}

		Type& operator*()
		{
			return *m_object;
		}

		Type& operator*() const
		{
			return *m_object;
		}

		Type* operator->()
		{
			return m_object;
		}

		Type* operator->() const
		{
			return m_object;
		}

		operator bool() const
		{
			return m_object != nullptr;
		}

		bool operator==(const e2::Ptr<Type>& rhs) const
		{
			return (m_object == rhs.m_object);
		}

		bool operator!=(const e2::Ptr<Type>& rhs) const
		{
			return !operator==(rhs);
		}

	protected:
		Type* m_object{};
	};




	struct E2_API ThreadInfo
	{
		/** The internal e2 id, sequential from 0 */
		uint32_t id;

		/** The stl id */
		std::thread::id cppId;

		e2::Name name;
	};

	/** Maximum number of utilized threads that can used e2::threadInfo */
	constexpr uint32_t maxPersistentThreads = 32;

	/** Designed to be used by persistent threads only. Usage elsewhere may exceed maxPersistentThreads and mess stuff up. */
	E2_API ThreadInfo const& threadInfo(e2::Name newName = e2::Name());

	/** unsigned 8 bit floating point type, 0.0f - 1.0f */
	class E2_API ufloat8
	{
	public:
		ufloat8()=default;
		explicit ufloat8(float value);
		
		ufloat8& operator=(float value);

		ufloat8& operator+=(e2::ufloat8 rhs);
		ufloat8& operator-=(e2::ufloat8 rhs);
		ufloat8& operator*=(e2::ufloat8 rhs);
		ufloat8& operator/=(e2::ufloat8 rhs);

		ufloat8& operator+=(float rhs);
		ufloat8& operator-=(float rhs);
		ufloat8& operator*=(float rhs);
		ufloat8& operator/=(float rhs);

		explicit operator float() const
		{
			return float(m_data) / 255.0f;
		}

		uint8_t data() const;

	protected:

		uint8_t m_data{};
	};


	/** Pointy-top hexagonal cube, offset coords default to odd-r */
	struct E2_API Hex
	{
		Hex();

		/** Axial coordinates (cube z will be inferred) */
		Hex(int32_t _x, int32_t y);

		/** Cube coordinates */
		Hex(int32_t _x, int32_t _y, int32_t _z);

		/** Cube fraction coordinates, will snap to grid  */
		Hex(glm::vec3 const& fraction);

		/** Planar coordinates*/
		Hex(glm::vec2 const& planar);

		/** Offset coordinates */
		Hex(glm::ivec2 const& offset);

		/** To planar coordinates*/
		glm::vec2 planarCoords() const;

		/** To offset coords */
		glm::ivec2 offsetCoords() const;

		/** to local coords */
		glm::vec3 localCoords() const;

		static glm::vec3 mix(Hex const& a, Hex const& b, float t);

		/** To fraction */
		glm::vec3 fraction() const;

		/** Writes a line from A to B into output, returns how many hexes written */
		static int32_t line(Hex const& a, Hex const& b, Hex* output, uint32_t outputSize);

		e2::StackVector<Hex, 6> neighbours();

		/** */
		static int32_t circle(Hex const& c, int32_t radius, std::vector<Hex>& output);

		static Hex zero();

		static Hex nw();

		static Hex ne();

		static Hex e();

		static Hex se();

		static Hex sw();

		static Hex w();

		static Hex dnw();

		static Hex dn();

		static Hex dne();

		static Hex dse();

		static Hex ds();

		static Hex dsw();

		// @todo maybe wanna SIMD these
		Hex& operator+=(Hex const& rhs);

		friend Hex operator+(Hex lhs, Hex const& rhs)
		{
			lhs += rhs;
			return lhs;
		}


		Hex& operator-=(Hex const& rhs);

		friend Hex operator-(Hex lhs, Hex const& rhs)
		{
			lhs -= rhs;
			return lhs;
		}

		Hex& operator*=(Hex const& rhs);
		friend Hex operator*(Hex lhs, Hex const& rhs)
		{
			lhs *= rhs;
			return lhs;
		}


		Hex& operator/=(Hex const& rhs);
		friend Hex operator/(Hex lhs, Hex const& rhs)
		{
			lhs /= rhs;
			return lhs;
		}


		int32_t length();

		static int32_t distance(Hex const& lhs, Hex const& rhs);

		int32_t x{};
		int32_t y{};
		int32_t z{};
	};

	inline bool operator== (Hex const& lhs, Hex const& rhs) noexcept
	{
		return lhs.x == rhs.x && lhs.y == rhs.y;
	}

	inline bool operator!= (Hex const& lhs, Hex const& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	inline bool operator< (Hex const& lhs, Hex const& rhs) noexcept
	{
		return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
	}


}

/** Make hexes hashable */
template <>
struct std::hash<e2::Hex>
{
	std::size_t operator()(const e2::Hex& hex) const
	{
		size_t h{};
		e2::hash_combine(h, hex.x);
		e2::hash_combine(h, hex.y);
		return h;
	}
};

/** Make pointers's hashable for unordered containers/hashmasp etc. */
template <typename UnderlyingType>
struct std::hash<e2::Ptr<UnderlyingType>>
{
	std::size_t operator()(const e2::Ptr<UnderlyingType>& ptr) const
	{
		return uintptr_t(ptr.get());
	}
};

/** Make names hashable */
template <>
struct std::hash<e2::Name>
{
	std::size_t operator()(const e2::Name& name) const
	{
		return name.index();
	}
};

/** make ivec2's hashable */
template <>
struct std::hash<glm::ivec2>
{
	std::size_t operator()(const glm::ivec2& vec2) const
	{
		size_t h{};
		e2::hash_combine(h, vec2.x);
		e2::hash_combine(h, vec2.y);
		return h;
	}
};

/** Make UUID's hashable for unordered containers/hashmasp etc. */
template <>
struct std::hash<e2::UUID>
{
	std::size_t operator()(const e2::UUID& uuid) const
	{
		return e2::hash(uuid);
	}
};

/** Makes e2::Name's formattable in std::format, and LogNotice etc. */
template <>
struct std::formatter<e2::Name>
{
	template<class ParseContext>
	constexpr ParseContext::iterator parse(ParseContext& ctx)
	{
		return ctx.begin();
	}

	template<class CmtContext>
	CmtContext::iterator format(e2::Name const& id, CmtContext& ctx) const
	{
		return std::format_to(ctx.out(), "{}", id.cstring());
	}
};

/*

template<>
struct std::formatter<QuotableString, char>
{
	bool quoted = false;

	template<class ParseContext>
	constexpr ParseContext::iterator parse(ParseContext& ctx)
	{

	}

	template<class FmtContext>
	FmtContext::iterator format(QuotableString s, FmtContext& ctx) const
	{

	}
};


*/


/** Makes glm types formattable in std::format, and LogNotice etc. */
template <>
struct std::formatter<glm::vec2>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto format(const glm::vec2& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "x={:.2f},y={:.2f}", vec.x, vec.y);
	}
};

template <>
struct std::formatter<glm::vec3>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto format(const glm::vec3& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "x={:.2f},y={:.2f},z={:.2f}", vec.x, vec.y, vec.z);
	}
};

template <>
struct std::formatter<glm::quat>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto format(const glm::quat& quat, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "w={:.2f},x={:.2f},y={:.2f},z={:.2f}", quat.w, quat.x, quat.y, quat.z);
	}
};

template <>
struct std::formatter<glm::ivec2>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto format(const glm::ivec2& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "x={},y={}", vec.x, vec.y);
	}
};

template <>
struct std::formatter<glm::uvec2>
{
	constexpr auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	auto format(const glm::uvec2& vec, std::format_context& ctx) const
	{
		return std::format_to(ctx.out(), "x={},y={}", vec.x, vec.y);
	}
};

#include "utils.generated.hpp"