#ifndef __UTIL_FGD_HPP__
#define __UTIL_FGD_HPP__

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>
class VFilePtrInternal;

namespace util
{
	namespace fgd
	{
		enum class ClassType : uint8_t
		{
			Base = 0u,
			Point,
			NPC,
			Solid,
			KeyFrame,
			Move,
			Filter,

			Unknown = std::numeric_limits<uint8_t>::max()
		};

		struct DataObject;
		using PDataObject = std::shared_ptr<DataObject>;

		class KeyValue
		{
		public:
			enum class Type : uint8_t
			{
				Void = 0u,
				String,
				Integer,
				Float,
				Choices,
				Flags,
				Axis,
				Angle,
				Color255,
				Color1,
				FilterClass,
				Material,
				NodeDest,
				NPCClass,
				Origin,
				PointEntityClass,
				Scene,
				SideList,
				Sound,
				Sprite,
				Studio,
				TargetDestination,
				TargetNameOrClass,
				TargetSource,
				VecLine,
				Vector,

				Unknown = std::numeric_limits<uint8_t>::max()
			};
			struct Choice
			{
				std::string name;
				std::string description;
				bool defaultOn;
			};
			KeyValue(const DataObject &obj);
			const std::string &GetName() const;
			const std::string &GetShortDescription() const;
			const std::string &GetLongDescription() const;
			const std::string &GetDefault() const;
			Type GetType() const;
			const std::unordered_map<std::string,Choice> &GetChoices() const;
		private:
			std::string m_name = {};
			std::string m_shortDesc = {};
			std::string m_longDesc = {};
			std::string m_default = {};
			Type m_type = Type::Unknown;

			// Only used if type is Type::Choices or Type::Flags
			std::unordered_map<std::string,Choice> m_choices = {};
		};

		struct Data;
		class ClassDefinition;
		using PClassDefinition = std::shared_ptr<ClassDefinition>;
		using WPClassDefinition = std::weak_ptr<ClassDefinition>;
		class ClassDefinition
			: public std::enable_shared_from_this<ClassDefinition>
		{
		public:
			ClassDefinition(const Data &fgdData,const DataObject &obj);
			const std::string &GetName() const;
			const std::string &GetDescription() const;
			const std::vector<WPClassDefinition> &GetBaseClasses() const;
			const std::vector<PDataObject> &GetProperties() const;
			const std::vector<KeyValue> &GetKeyValues() const;
			const std::vector<KeyValue> &GetInputs() const;
			const std::vector<KeyValue> &GetOutputs() const;
			ClassType GetType() const;

			// Finds the specified keyvalue located in either this class, or one of this class' base classes
			const KeyValue *FindKeyValue(const Data &fgdData,const std::string &name) const;
			// Finds the specified input located in either this class, or one of this class' base classes
			const KeyValue *FindInput(const Data &fgdData,const std::string &name) const;
			// Finds the specified output located in either this class, or one of this class' base classes
			const KeyValue *FindOutput(const Data &fgdData,const std::string &name) const;
		private:
			enum class KeyValueType : uint8_t
			{
				KeyValue = 0u,
				Input,
				Output
			};
			const KeyValue *FindKeyValue(const Data &fgdData,KeyValueType type,const std::string &name) const;

			std::string m_name = {};
			std::string m_description = {};
			std::vector<WPClassDefinition> m_baseClasses = {};
			std::vector<PDataObject> m_properties = {};
			std::vector<KeyValue> m_keyValues = {};
			std::vector<KeyValue> m_inputs = {};
			std::vector<KeyValue> m_outputs = {};
			ClassType m_type = ClassType::Unknown;
		};

		struct Data
		{
			std::pair<int32_t,int32_t> mapSize;
			std::vector<std::string> includes;
			std::unordered_map<std::string,PClassDefinition> classDefinitions;
		};

		struct DataObject
		{
			std::string name;
			std::vector<std::string> arguments; // Function arguments
			std::vector<PDataObject> parameters; // Class parameters
			std::vector<PDataObject> attributes; // Class fields (description, etc.)
			std::vector<PDataObject> children; // Class child elements
		};
		std::optional<util::fgd::Data> load_fgd(const std::string &fileName,const std::function<std::shared_ptr<VFilePtrInternal>(const std::string&)> &fileFactory,std::unordered_map<std::string,Data> &fgdCache);
		std::optional<util::fgd::Data> load_fgd(const std::string &fileName,const std::function<std::shared_ptr<VFilePtrInternal>(const std::string&)> &fileFactory);
		std::optional<util::fgd::Data> load_fgd(const std::string &fileName,std::unordered_map<std::string,Data> &fgdCache);
		std::optional<util::fgd::Data> load_fgd(const std::string &fileName);
	};
};

#endif
