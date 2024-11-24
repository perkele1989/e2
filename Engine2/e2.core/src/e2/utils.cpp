
#include "e2/utils.hpp"

#include "e2/log.hpp"

#define UUID_SYSTEM_GENERATOR
#include "uuid.h"

#include <glm/gtc/integer.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "utf8.h"
//#include "utf8/cpp11.h"
#include "utf8/cpp17.h"


#include <fstream>
#include <sstream>
#include <map>

#if defined(E2_DEVELOPMENT)
#include <stacktrace>
#endif


namespace
{
	e2::Arena<e2::ManagedBlock> blockArena(e2::managedBlockArenaSize);
}

e2::ManagedBlock* e2::createBlock()
{
	return ::blockArena.create();
}

void e2::destroyBlock(e2::ManagedBlock* block)
{
	::blockArena.destroy(block);
}


bool e2::readFile(std::string const& path, std::string& outData)
{
	std::ifstream handle(path);
	if (!handle)
		return false;

	std::ostringstream buffer;
	buffer << handle.rdbuf();
	outData = buffer.str();

	return true;
}



namespace
{
	thread_local e2::ThreadInfo thisThread{};
	thread_local bool thisThreadInitialized{false};

	std::mutex  threadIdMutex;
	uint32_t nextThreadId{};
}

e2::ThreadInfo const& e2::threadInfo(e2::Name newName)
{
	if (!::thisThreadInitialized)
	{
		uint32_t newId{};
		{
			std::scoped_lock lock(::threadIdMutex);

			if (::nextThreadId > maxPersistentThreads)
			{
				LogWarning("maxPersistentThreads exceeded.");
			}

			newId = ::nextThreadId++;
		}

		
		
		::thisThread.id = newId;
		::thisThread.cppId = std::this_thread::get_id();
		::thisThread.name = newName;
		::thisThreadInitialized = true;
	}

	return ::thisThread;
}

glm::mat4 e2::recompose(glm::vec3 const& translation, glm::vec3 const& scale, glm::vec3 const& skew, glm::vec4 const& perspective, glm::quat const& rotation)
{
	glm::mat4 m = glm::mat4(1.f);

	m[0][3] = perspective.x;
	m[1][3] = perspective.y;
	m[2][3] = perspective.z;
	m[3][3] = perspective.w;

	m *= glm::translate(translation);
	m *= glm::mat4_cast(rotation);

	if (skew.x) {
		glm::mat4 tmp{ 1.f };
		tmp[2][1] = skew.x;
		m *= tmp;
	}

	if (skew.y) {
		glm::mat4 tmp{ 1.f };
		tmp[2][0] = skew.y;
		m *= tmp;
	}

	if (skew.z) {
		glm::mat4 tmp{ 1.f };
		tmp[1][0] = skew.z;
		m *= tmp;
	}

	m *= glm::scale(scale);

	return m;
}

glm::vec2 e2::rotate2d(glm::vec2 const& vec, float angleDegrees)
{
	glm::vec2 out;
	float rad = glm::radians(angleDegrees);
	out.x = vec.x * glm::cos(rad) - vec.y * glm::sin(rad);
	out.y = vec.x * glm::sin(rad) + vec.y * glm::cos(rad);
	return out;
}

float e2::radiansBetween(glm::vec3 const& a, glm::vec3 const& b)
{
	if (a == b)
		return 0.0f;

	glm::vec2 a2 = { a.x, a.z };
	glm::vec2 b2 = { b.x, b.z };
	glm::vec2 na = glm::normalize(a2 - b2);

	glm::vec2 nb = glm::normalize(glm::vec2(0.0f, -1.0f));

	return glm::orientedAngle(nb, na);
}

size_t e2::hash(UUID const& id)
{
	return std::hash<uuids::uuid>{}(uuids::uuid(id.data));
}

bool e2::nearlyEqual(float x, float y, float tresh)
{
	return glm::abs(y - x) <= tresh;
}

bool e2::intersect(glm::vec2 p, glm::vec2 boxOffset, glm::vec2 boxSize)
{
	return p.x >= boxOffset.x && p.x <= boxOffset.x + boxSize.x
		&& p.y >= boxOffset.y && p.y <= boxOffset.y + boxSize.y;
}

namespace
{
	bool initialized = false;
	std::random_device rd;
	std::mt19937 gen;

	void assertInitialized()
	{
		if (initialized)
			return;

		initialized = true;
		gen = std::mt19937(rd());
	}



}

bool e2::randomBool()
{
	assertInitialized();

	return (bool)std::uniform_int_distribution<>(0, 1)(::gen);
}

int64_t e2::randomInt(int64_t min, int64_t max)
{
	assertInitialized();

	return std::uniform_int_distribution<int64_t>(min, max)(::gen);
}

float e2::randomFloat(float min, float max)
{
	assertInitialized();

	return std::uniform_real_distribution<float>(min, max)(::gen);
}

double e2::randomDouble(double min, double max)
{
	assertInitialized();

	return std::uniform_real_distribution<double>(min, max)(::gen);
}

glm::ivec2 e2::randomIvec2(glm::ivec2 min, glm::ivec2 max)
{
	return glm::ivec2((int32_t)randomInt(min.x, max.x), (int32_t)randomInt(min.y, max.y));
}

glm::vec2 e2::randomVec2(glm::vec2 min, glm::vec2 max)
{
	return glm::vec2(randomFloat(min.x, max.x), randomFloat(min.y, max.y));
}

glm::vec3 e2::randomVec3(glm::vec3 min, glm::vec3 max)
{
	return glm::vec3(randomFloat(min.x, max.x), randomFloat(min.y, max.y), randomFloat(min.z, max.z));
}

glm::vec2 e2::randomOnUnitCircle()
{
	//glm::vec2 fwd{ 1.0f, 0.0f};
	//return glm::rotate(fwd, glm::radians(e2::randomFloat(0.0f, 359.9999f)));
	float theta = e2::randomFloat(0.0f, 1.0f) * 2.0f * glm::pi<float>();
	return { glm::cos(theta), glm::sin(theta) };
}

glm::vec2 e2::randomInUnitCircle()
{

	//r = R * sqrt(random())
	//theta = random() * 2 * PI

	//(Assuming random() gives a value between 0 and 1 uniformly)

	//If you want to convert this to Cartesian coordinates, you can do

	//x = centerX + r * cos(theta)
	//y = centerY + r * sin(theta)

	float r = glm::sqrt(e2::randomFloat(0.0f, 1.0f));
	float theta = e2::randomFloat(0.0f, 1.0f) * 2.0f * glm::pi<float>();
	return { r * glm::cos(theta), r * glm::sin(theta) };


	//return randomOnUnitCircle() * randomFloat(0.0f, 1.f);
}

glm::vec3 e2::randomOnUnitSphere()
{
	return glm::normalize(randomVec3({ -1.0f, -1.0f, -1.0f }, { 1.0f, 1.0f , 1.0f}));
}

glm::vec3 e2::randomInUnitSphere()
{
	return randomOnUnitSphere() * randomFloat(0.0f, 1.0f);
}

std::string e2::replace(std::string const& needle, std::string const& replacement, std::string const& haystack)
{
	std::string newString = haystack;
	size_t finder = newString.find(needle);
	while (finder != std::string::npos)
	{
		if (newString.size() < needle.size())
		{
			break;
		}

		newString = newString.replace(finder, needle.size(), replacement);
		finder = newString.find(needle);
	}

	return newString;
}

E2_API std::string e2::trim(std::string const& input)
{
	std::string s(input);

	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
		return !std::isspace(ch);
	}));

	s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
		return !std::isspace(ch);
	}).base(), s.end());

	return s;
}

E2_API std::string e2::trim(std::string const& input, std::vector<char> const& characters)
{
	std::string s(input);

	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [&characters](unsigned char ch) {
		bool match = std::find(characters.begin(), characters.end(), ch) != characters.end();
		return !match;
	}));

	s.erase(std::find_if(s.rbegin(), s.rend(), [&characters](unsigned char ch) {
		bool match = std::find(characters.begin(), characters.end(), ch) != characters.end();
		return !match;
	}).base(), s.end());

	return s;
}

E2_API std::vector<std::string> e2::split(std::string const& input, char delimiter)
{
	std::vector<std::string> output;
	std::string delimiterString;
	delimiterString.push_back(delimiter);

	size_t start{};
	size_t end{};

	end = input.find(delimiter, start);
	while (end != std::string::npos)
	{
		std::string newString = input.substr(start, end - start);
		if (newString.size() > 0 && newString != delimiterString)
		{
			output.push_back(newString);
		}

		start = end + 1;
		end = input.find(delimiter, start);
	}

	std::string restString = input.substr(start);
	if (restString.size() > 0 && restString != delimiterString)
	{
		output.push_back(restString);
	}

	return output;
}


std::string e2::toLower(std::string const& input)
{
	std::string output = input;
	std::transform(output.begin(), output.end(), output.begin(), [](unsigned char c) { return std::tolower(c); });
	return output;
}

std::string e2::utf32to8(std::u32string const& utf32)
{
	return utf8::utf32to8(utf32);
}

std::u32string e2::utf8to32(std::string const& utf8)
{
	return utf8::utf8to32(utf8);
}

std::string e2::genericToUtf8(std::string const& generic)
{
	std::string temp;
	utf8::replace_invalid(generic.begin(), generic.end(), back_inserter(temp));
	return temp;
}

bool e2::writeFile(std::string const& path, std::string const& data)
{
	std::ofstream handle(path);
	if (!handle)
		return false;

	handle << data;

	return true;

}

E2_API bool e2::readFileWithIncludes(std::string const& path, std::string& resolvedData)
{
	std::string currentBuffer;
	if (!readFile(path, currentBuffer))
	{
		LogError("Failed to read file: \"%s\"", path);
		return false;
	}

	std::vector<std::string> lines = e2::split(currentBuffer, '\n');
	for (std::string const& line : lines)
	{
		std::string trimmed = e2::trim(line);
		if (trimmed.starts_with("#include") && trimmed.size() > 8)
		{
			std::string includePath = e2::trim(e2::trim(trimmed.substr(8)), {'<', '>', '"'});
			if (!readFileWithIncludes(includePath, resolvedData))
			{
				LogError("Failed to read file: \"%s\", included from \"%s\"", includePath, path);
				return false;
			}
		}
		else
		{
			resolvedData.append(line + "\n");
		}
	}

	return true;
}


uint32_t e2::calculateMipLevels(glm::uvec2 const& resolution)
{
	return uint32_t(1 + glm::floor(glm::log2(glm::max(resolution.x, resolution.y))));
}


e2::Object* e2::createDynamic(e2::Name typeName)
{
	e2::Type* type = e2::Type::fromName(typeName);
	if (!type)
	{
		LogError("Could not resolve type {}", typeName.string());
		return nullptr;
	}

	return type->create();
}


e2::UUID e2::UUID::generate()
{
	e2::UUID returner;
	returner.data = uuids::uuid_system_generator{}().get_data();
	return returner;
}

e2::UUID::UUID()
{

}

std::string e2::UUID::string() const
{
	return uuids::to_string(uuids::uuid(data));
}


void e2::UUID::write(e2::IStream& destination) const
{
	destination.write(data.data(), 16);
}

bool e2::UUID::read(e2::IStream& source)
{
	uint8_t const *dataPtr = source.read(16);
	if (!dataPtr)
		return false;

	memcpy(data.data(), dataPtr, 16);

	return true;
}

bool e2::UUID::valid()
{
	static const e2::UUID null;
	return !(data == null.data);
}

e2::UUID::UUID(std::string const& str)
{
	data = uuids::uuid::from_string(str).value().get_data();
}




namespace
{
	// @todo unordered_map instead? takes more memory but is faster 
	std::unordered_map<std::string, uint32_t> nameMap;
	char* nameBuffer{};
	uint32_t nameCursor{};

	// @todo this isnt threadsafe, make sure we create a name on mainthread on startup, before we trigger any threads.
	// We dont have to clean this up, because it by-design stays until application termination, and OS will clean up when process dies.
	void assertNameBuffer()
	{
		if (!::nameBuffer)
		{
			::nameBuffer = new char[e2::nameBufferLength];

			::nameMap.reserve(e2::nameBufferLength / 4);

			// First name (index 0) is always "None", to give us a "null" index 
			e2::Name("None");
		}
	}
}


e2::Name::Name(char const* c)
	: e2::Name(std::string(c))
{

}

e2::Name::Name()
{
#if defined(E2_DEVELOPMENT)
	m_debugName = "None";
#endif
}

e2::Name::Name(std::string_view v)
	: e2::Name(std::string(v))
{
	
}

e2::Name::Name(std::string const& s)
{
	assertNameBuffer();

	auto finder = ::nameMap.find(s);
	if (finder == ::nameMap.end())
	{
		if (::nameCursor + s.size() + 1 >= e2::nameBufferLength)
		{
			m_index = 0;
			LogError("e2::nameBufferLength reached");
			return;
		}

		m_index = ::nameCursor;
		strcpy_s(&::nameBuffer[m_index], s.size() + 1, s.c_str());
		//::nameBuffer[m_index + s.size()] = '\0';
		::nameCursor += (uint32_t)s.size() + 1;

		::nameMap[s] = m_index;
	}
	else
	{
		m_index = finder->second;
	}

#if defined(E2_DEVELOPMENT)
	m_debugName = string();
#endif
}

e2::Name::~Name()
{

}

void e2::Name::write(e2::IStream& destination) const
{
	destination << string();
}

bool e2::Name::read(e2::IStream& source)
{
	std::string buff;
	source >> buff;
	*this = buff;

	return true;
}

std::string_view e2::Name::view() const
{
	return std::string_view(&::nameBuffer[m_index]);
}

std::string e2::Name::string() const
{
	return std::string(&::nameBuffer[m_index]);
}

char const* e2::Name::cstring() const
{
	return &::nameBuffer[m_index];
}

namespace
{
	std::map<e2::Name, e2::Type*> typeIndex;
}

void e2::Type::registerType()
{
	::typeIndex[fqn] = this;
	//LogNotice("Registered type: {}", fqn);
}

e2::Type* e2::Type::fromName(e2::Name name)
{
	auto finder = ::typeIndex.find(name);
	if (finder == ::typeIndex.end())
	{
		LogError("failed to find type by name: {}", name.cstring());
		return nullptr;
	}

	return finder->second;
}

bool e2::Type::inherits(e2::Name baseType, bool includeAncestors /*= true*/)
{
	if (includeAncestors)
	{
		return allBases.contains(baseType);
	}
	else
	{
		return bases.contains(baseType);
	}
}

bool e2::Type::inheritsOrIs(e2::Name baseType, bool includeAncestors /*= true*/)
{
	return baseType == fqn || inherits(baseType, includeAncestors);
}

std::set<e2::Type*> e2::Type::findChildTypes()
{
	std::set<e2::Type*> returner;
	for (std::pair<e2::Name, e2::Type*> pair : ::typeIndex)
	{
		if (pair.second->inherits(fqn, true))
			returner.insert(pair.second);
	}

	return returner;
}

#if defined(E2_DEVELOPMENT)

namespace
{
	struct ObjectTracker
	{
		e2::Object* object{};
		std::string callStack;
	};
	
	std::map<e2::Object*, ObjectTracker> trackedObjects;
	std::mutex trackedMutex;
}

void e2::printLingeringObjects()
{
	if (::trackedObjects.size() == 0)
		return;

	LogError("{} lingering objects:", ::trackedObjects.size());
	for (std::pair<e2::Object* const, ObjectTracker>& obj : ::trackedObjects)
	{
		LogError("{} from:", uint64_t(obj.first));
		LogError("{}\n", obj.second.callStack);
	}
}

#endif


e2::ManagedObject::ManagedObject()
	: e2::Object()
{
	m_block = e2::createBlock();
	m_block->objPtr.store(reinterpret_cast<uintptr_t>(this));
	m_block->blockIncrement();
}

e2::ManagedObject::~ManagedObject()
{
	m_block->objPtr.store(reinterpret_cast<uintptr_t>(nullptr));
	m_block->blockDecrement();
}


namespace
{
	uint8_t internalFrameIndex = 0;
	e2::StackVector<e2::Ptr<e2::ManagedObject>, 16384> keptAround[4];
}


void e2::ManagedObject::keepAround()
{
	::keptAround[internalFrameIndex].push(e2::Ptr<e2::ManagedObject>(this));
}

e2::ManagedObject* e2::ManagedBlock::object()
{
	return reinterpret_cast<e2::ManagedObject*>(objPtr.load());
}

void e2::ManagedBlock::objIncrement()
{
	objRef++;
}

void e2::ManagedBlock::objDecrement()
{
	--objRef;
	if (objRef <= 0 && pendingKill)
		e2::destroy(object());
}

void e2::ManagedBlock::blockIncrement()
{
	blockRef++;
}

//#include "e2/managedptr_impl.inl"
//template class e2::Ptr<e2::ManagedObject>;

void e2::ManagedBlock::blockDecrement()
{
	if (--blockRef <= 0)
	{
		e2::destroyBlock(this);
	}
}

bool e2::ManagedBlock::isPendingKill()
{
	return pendingKill;
}

void e2::ManagedBlock::flagPendingKill()
{
	pendingKill = true;
	if (objRef <= 0)
		e2::destroy(object());
}

e2::Object::Object()
{
#if defined(E2_DEVELOPMENT)
	std::scoped_lock lock(trackedMutex);
	ObjectTracker tracker;
	tracker.object = this;
	tracker.callStack = std::to_string(std::stacktrace::current());
	::trackedObjects[this] = tracker;
#endif
}

e2::Object::~Object()
{
#if defined(E2_DEVELOPMENT)
	std::scoped_lock lock(trackedMutex);
	::trackedObjects.erase(this);
#endif
}

void e2::ManagedObject::keepAroundTick()
{
	::internalFrameIndex++;
	if (::internalFrameIndex >= 4)
		::internalFrameIndex = 0;

	for (uint32_t i = 0; i < ::keptAround[::internalFrameIndex].size(); i++)
	{
		::keptAround[::internalFrameIndex][i] = nullptr;
	}

	::keptAround[::internalFrameIndex].resize(0);
}

void e2::ManagedObject::keepAroundPrune()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		for (uint32_t h = 0; h < ::keptAround[i].size(); h++)
		{
			::keptAround[i][h] = nullptr;
		}

		::keptAround[i].resize(0);
	}
}

e2::ufloat8::ufloat8(float value)
{
	*this = value;
}


e2::ufloat8& e2::ufloat8::operator+=(e2::ufloat8 rhs)
{
	float floatValue = float(*this) + float(rhs);
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator-=(e2::ufloat8 rhs)
{
	float floatValue = float(*this) - float(rhs);
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator*=(e2::ufloat8 rhs)
{
	float floatValue = float(*this) * float(rhs);
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator/=(e2::ufloat8 rhs)
{
	float floatValue = float(*this) / float(rhs);
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator+=(float rhs)
{
	float floatValue = float(*this) + rhs;
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator-=(float rhs)
{
	float floatValue = float(*this) - rhs;
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator*=(float rhs)
{
	float floatValue = float(*this) * rhs;
	*this = floatValue;
	return *this;
}

e2::ufloat8& e2::ufloat8::operator/=(float rhs)
{
	float floatValue = float(*this) / rhs;
	*this = floatValue;
	return *this;
}


uint8_t e2::ufloat8::data() const
{
	return m_data;
}

e2::ufloat8& e2::ufloat8::operator=(float value)
{
	m_data = uint8_t(glm::clamp(value, 0.0f, 1.0f) * 255.f);
	return *this;
}


e2::Hex::Hex(glm::vec3 const& frac)
{
	x = int32_t(glm::round(frac.x));
	y = int32_t(glm::round(frac.y));
	z = int32_t(glm::round(frac.z));

	// Renormalize to hex-space
	float xd = glm::abs(x - frac.x);
	float yd = glm::abs(y - frac.y);
	float zd = glm::abs(z - frac.z);

	if (xd > yd && xd > zd)
	{
		x = -y - z;
	}
	else if (yd > zd)
	{
		y = -x - z;
	}
	else
	{
		z = -x - y;
	}
}

e2::Hex::Hex(glm::ivec2 const& offset)
{
	x = offset.x - (offset.y - (offset.y & 1)) / 2;
	y = offset.y;
	z = -x - y;
}

glm::vec2 e2::Hex::planarCoords() const
{
	glm::vec2 r;
	r.x = (glm::sqrt(3.f) * float(x) + glm::sqrt(3.f) / 2.f * y);
	r.y = (3.f / 2.f * y);
	return r;
}

glm::ivec2 e2::Hex::offsetCoords() const
{
	glm::ivec2 offset;
	offset.x = x + (y - (y & 1)) / 2;
	offset.y = y;

	return offset;
}

glm::vec3 e2::Hex::localCoords() const
{
	glm::vec2 planar = planarCoords();
	glm::vec3 returner{};
	returner.x = planar.x;
	returner.z = planar.y;

	return returner;
}

e2::Hex::Hex(glm::vec2 const& planarCoords)
{
	glm::vec3 hexFraction;

	hexFraction.x = (glm::sqrt(3.f) / 3.f * planarCoords.x - 1.f / 3.f * planarCoords.y);
	hexFraction.y = (2.f / 3.f * planarCoords.y);
	hexFraction.z = -hexFraction.x - hexFraction.y;

	*this = Hex(hexFraction);
}

e2::Hex::Hex(int32_t _x, int32_t _y)
	: x(_x)
	, y(_y)
	, z(-x - y)
{

}

e2::Hex::Hex(int32_t _x, int32_t _y, int32_t _z)
	: x(_x)
	, y(_y)
	, z(_z)
{

}

e2::Hex::Hex()
{

}

glm::vec3 e2::Hex::mix(Hex const& a, Hex const& b, float t)
{

	glm::vec3 va = a.fraction();
	glm::vec3 vb = b.fraction();
	return glm::mix(va, vb, t);


	/* return {
			glm::mix((float)a.x, (float)b.x, t),
			glm::mix((float)a.y, (float)b.y, t),
			glm::mix((float)a.z, (float)b.z, t),
		};
		*/
}

glm::vec3 e2::Hex::fraction() const
{
	return { (float)x, (float)y, (float)z };
}

int32_t e2::Hex::line(Hex const& a, Hex const& b, Hex* output, uint32_t outputSize)
{
	int32_t dist = distance(a, b);
	dist = glm::clamp(dist, 0, int32_t(outputSize));

	if (dist == 0)
	{
		output[0] = a;
		return 1;
	}

	// nudge to make lines more predictable and straight (due to how rounding works near integer edges)
	glm::vec3 nudgeA{ float(a.x) + 1e-6f, (a.y) + 1e-6f, (a.z) - 2e-6f };
	glm::vec3 nudgeB{ float(b.x) + 1e-6f, (b.y) + 1e-6f, (b.z) - 2e-6f };

	float step = 1.0f / dist;
	for (int32_t i = 0; i < dist; i++)
	{
		glm::vec3 l = mix(nudgeA, nudgeB, step * float(i));
		output[i] = Hex(l);
	}

	return dist;
}

e2::StackVector<e2::Hex, 6> e2::Hex::neighbours()
{
	e2::StackVector<e2::Hex, 6> returner;
	returner.push(*this + nw());
	returner.push(*this + ne());
	returner.push(*this + e());
	returner.push(*this + se());
	returner.push(*this + sw());
	returner.push(*this + w());

	return returner;
}

int32_t e2::Hex::circle(Hex const& c, int32_t radius, std::vector<Hex> &output)
{
	/*
	var results = []
	for each -N ≤ q ≤ +N:
		for each max(-N, -q-N) ≤ r ≤ min(+N, -q+N):
			var s = -q-r
			results.append(cube_add(center, Cube(q, r, s)))
	*/

	int32_t i = 0;
	for (int32_t q = -radius; q <= radius; q++)
		for (int32_t r = glm::max(-radius, -q - radius); r <= glm::min(radius, -q + radius); r++)
			output.push_back( c + e2::Hex(q, r, -q-r));

	return i;
}

e2::Hex e2::Hex::zero()
{
	return {};
}

e2::Hex e2::Hex::nw()
{
	return { 0, -1, 1 };
}

e2::Hex e2::Hex::ne()
{
	return { 1, -1, 0 };
}

e2::Hex e2::Hex::e()
{
	return { 1, 0, -1 };
}

e2::Hex e2::Hex::se()
{
	return { 0, 1, -1 };
}

e2::Hex e2::Hex::sw()
{
	return { -1, 1, 0 };
}

e2::Hex e2::Hex::w()
{
	return { -1, 0, 1 };
}

e2::Hex e2::Hex::dnw()
{
	return { -1, -1, 2 };
}

e2::Hex e2::Hex::dn()
{
	return { 1, -2, 1 };
}

e2::Hex e2::Hex::dne()
{
	return { 2, -1, -1 };
}

e2::Hex e2::Hex::dse()
{
	return { 1, 1, -2 };
}

e2::Hex e2::Hex::ds()
{
	return { -1, 2, -1 };
}

e2::Hex e2::Hex::dsw()
{
	return { -2, 1, 1 };
}

e2::Hex& e2::Hex::operator+=(Hex const& rhs)
{
	x = x + rhs.x;
	y = y + rhs.y;
	z = z + rhs.z;

	return *this;
}

e2::Hex& e2::Hex::operator-=(Hex const& rhs)
{
	x = x - rhs.x;
	y = y - rhs.y;
	z = z - rhs.z;

	return *this;
}


e2::Hex& e2::Hex::operator*=(Hex const& rhs)
{
	x = x * rhs.x;
	y = y * rhs.y;
	z = z * rhs.z;

	return *this;
}

e2::Hex& e2::Hex::operator/=(Hex const& rhs)
{
	x = x / rhs.x;
	y = y / rhs.y;
	z = z / rhs.z;

	return *this;
}


int32_t e2::Hex::length()
{
	return distance({}, *this);
}

int32_t e2::Hex::distance(Hex const& lhs, Hex const& rhs)
{
	Hex c = lhs - rhs;
	return (glm::abs(c.x) + glm::abs(c.y) + glm::abs(c.z)) / 2;
}

int32_t e2::Hex::distance(glm::ivec2 const& lhs, glm::ivec2 const& rhs)
{
	return distance(Hex(lhs), Hex(rhs));
}
