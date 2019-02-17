/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "util_fgd.hpp"
#include <iostream>
#include <stack>
#include <sstream>
#include <assert.h>
#include <fsys/filesystem.h>
#include <sharedutils/util.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util_markup_file.hpp>
#include <sharedutils/datastream.h>

#pragma comment(lib,"util.lib")
#pragma comment(lib,"mathutil.lib")
#pragma comment(lib,"vfilesystem.lib")

util::fgd::KeyValue::KeyValue(const DataObject &obj)
	: m_name{obj.name}
{
	auto numAttrs = obj.attributes.size();
	auto idx = 0u;
	if(numAttrs > 0u && (ustring::compare(obj.attributes.front()->name,"input",false) == true || ustring::compare(obj.attributes.front()->name,"output",false) == true))
		++idx;
	if(numAttrs > idx)
	{
		m_shortDesc = obj.attributes.at(idx++)->name;
		if(numAttrs > idx)
		{
			m_default = obj.attributes.at(idx++)->name;
			if(numAttrs > idx)
				m_longDesc = obj.attributes.at(idx++)->name;
		}
	}
	if(obj.arguments.empty() == false)
	{
		auto &strType = obj.arguments.front();
		if(ustring::compare(strType,"void",false))
			m_type = Type::Void;
		else if(ustring::compare(strType,"string",false))
			m_type = Type::String;
		else if(ustring::compare(strType,"integer",false))
			m_type = Type::Integer;
		else if(ustring::compare(strType,"float",false))
			m_type = Type::Float;
		else if(ustring::compare(strType,"choices",false))
			m_type = Type::Choices;
		else if(ustring::compare(strType,"flags",false))
			m_type = Type::Flags;
		else if(ustring::compare(strType,"axis",false))
			m_type = Type::Axis;
		else if(ustring::compare(strType,"angle",false))
			m_type = Type::Angle;
		else if(ustring::compare(strType,"color255",false))
			m_type = Type::Color255;
		else if(ustring::compare(strType,"color1",false))
			m_type = Type::Color1;
		else if(ustring::compare(strType,"filterclass",false))
			m_type = Type::FilterClass;
		else if(ustring::compare(strType,"material",false))
			m_type = Type::Material;
		else if(ustring::compare(strType,"node_dest",false))
			m_type = Type::NodeDest;
		else if(ustring::compare(strType,"npcclass",false))
			m_type = Type::NPCClass;
		else if(ustring::compare(strType,"origin",false))
			m_type = Type::Origin;
		else if(ustring::compare(strType,"pointentityclass",false))
			m_type = Type::PointEntityClass;
		else if(ustring::compare(strType,"scene",false))
			m_type = Type::Scene;
		else if(ustring::compare(strType,"sidelist",false))
			m_type = Type::SideList;
		else if(ustring::compare(strType,"sound",false))
			m_type = Type::Sound;
		else if(ustring::compare(strType,"sprite",false))
			m_type = Type::Sprite;
		else if(ustring::compare(strType,"studio",false))
			m_type = Type::Studio;
		else if(ustring::compare(strType,"target_destination",false))
			m_type = Type::TargetDestination;
		else if(ustring::compare(strType,"target_name_or_class",false))
			m_type = Type::TargetNameOrClass;
		else if(ustring::compare(strType,"target_source",false))
			m_type = Type::TargetSource;
		else if(ustring::compare(strType,"vecline",false))
			m_type = Type::VecLine;
		else if(ustring::compare(strType,"vector",false))
			m_type = Type::Vector;
	}
	if(m_type == Type::Choices || m_type == Type::Flags)
	{
		for(auto &child : obj.children)
		{
			std::string value {};
			std::string desc {};
			auto defaultOn = false;
			if(child->attributes.size() > 0u)
			{
				value = child->attributes.at(0u)->name;
				if(child->attributes.size() > 1u)
				{
					desc = child->attributes.at(1u)->name;
					if(child->attributes.size() > 2u && m_type == Type::Flags)
						defaultOn = util::to_boolean(child->attributes.at(2u)->name);
				}
			}
			m_choices.insert(std::make_pair(child->name,Choice{
				value,desc,defaultOn
			}));
		}
	}
}
const std::string &util::fgd::KeyValue::GetName() const {return m_name;}
const std::string &util::fgd::KeyValue::GetShortDescription() const {return m_shortDesc;}
const std::string &util::fgd::KeyValue::GetLongDescription() const {return m_longDesc;}
const std::string &util::fgd::KeyValue::GetDefault() const {return m_default;}
util::fgd::KeyValue::Type util::fgd::KeyValue::GetType() const {return m_type;}
const std::unordered_map<std::string,util::fgd::KeyValue::Choice> &util::fgd::KeyValue::GetChoices() const {return m_choices;}

util::fgd::ClassDefinition::ClassDefinition(const Data &fgdData,const DataObject &obj)
{
	if(obj.attributes.size() > 0u)
	{
		m_name = obj.attributes.at(0u)->name;
		if(obj.attributes.size() > 1u)
			m_description = obj.attributes.at(1u)->name;
	}
	if(ustring::compare(obj.name,"@BaseClass",false))
		m_type = ClassType::Base;
	else if(ustring::compare(obj.name,"@PointClass",false))
		m_type = ClassType::Point;
	else if(ustring::compare(obj.name,"@NPCClass",false))
		m_type = ClassType::NPC;
	else if(ustring::compare(obj.name,"@SolidClass",false))
		m_type = ClassType::Solid;
	else if(ustring::compare(obj.name,"@KeyFrameClass",false))
		m_type = ClassType::KeyFrame;
	else if(ustring::compare(obj.name,"@MoveClass",false))
		m_type = ClassType::Move;
	else if(ustring::compare(obj.name,"@FilterClass",false))
		m_type = ClassType::Filter;

	m_properties = obj.parameters;
	auto itBase = std::find_if(obj.parameters.begin(),obj.parameters.end(),[](const util::fgd::PDataObject &obj) {
		return ustring::compare(obj->name,"base",false);
	});
	if(itBase != obj.parameters.end())
	{
		auto &args = (*itBase)->arguments;
		m_baseClasses.reserve(args.size());
		for(auto arg : args)
		{
			ustring::to_lower(arg);
			auto itBaseDef = fgdData.classDefinitions.find(arg);
			if(itBaseDef != fgdData.classDefinitions.end())
				m_baseClasses.push_back(itBaseDef->second);
		}
	}
	auto numChildren = obj.children.size();
	m_keyValues.reserve(numChildren);
	m_inputs.reserve(numChildren);
	m_outputs.reserve(numChildren);
	for(auto &child : obj.children)
	{
		if(child->attributes.empty() == false)
		{
			auto &attr = child->attributes.front();
			if(ustring::compare(attr->name,"input",false))
			{
				m_inputs.push_back({*child});
				continue;
			}
			if(ustring::compare(attr->name,"output",false))
			{
				m_outputs.push_back({*child});
				continue;
			}
		}
		m_keyValues.push_back({*child});
	}
}
const std::string &util::fgd::ClassDefinition::GetName() const {return m_name;}
const std::string &util::fgd::ClassDefinition::GetDescription() const {return m_description;}
const std::vector<util::fgd::WPClassDefinition> &util::fgd::ClassDefinition::GetBaseClasses() const {return m_baseClasses;}
const std::vector<util::fgd::PDataObject> &util::fgd::ClassDefinition::GetProperties() const {return m_properties;}
const std::vector<util::fgd::KeyValue> &util::fgd::ClassDefinition::GetKeyValues() const {return m_keyValues;}
const std::vector<util::fgd::KeyValue> &util::fgd::ClassDefinition::GetInputs() const {return m_inputs;}
const std::vector<util::fgd::KeyValue> &util::fgd::ClassDefinition::GetOutputs() const {return m_outputs;}
util::fgd::ClassType util::fgd::ClassDefinition::GetType() const {return m_type;}
const util::fgd::KeyValue *util::fgd::ClassDefinition::FindKeyValue(const Data &fgdData,KeyValueType type,const std::string &name) const
{
	auto &keyValueList = (type == KeyValueType::KeyValue) ? m_keyValues : (type == KeyValueType::Input) ? m_inputs : m_outputs;
	auto it = std::find_if(keyValueList.begin(),keyValueList.end(),[&name](const util::fgd::KeyValue &keyValue) {
		return ustring::compare(keyValue.GetName(),name,false);
	});
	if(it != keyValueList.end())
		return &(*it);
	for(auto &wpClass : m_baseClasses)
	{
		if(wpClass.expired())
			continue;
		auto *pKeyValue = wpClass.lock()->FindKeyValue(fgdData,type,name);
		if(pKeyValue != nullptr)
			return pKeyValue;
	}
	return nullptr;
}
const util::fgd::KeyValue *util::fgd::ClassDefinition::FindKeyValue(const Data &fgdData,const std::string &name) const {return FindKeyValue(fgdData,KeyValueType::KeyValue,name);}
const util::fgd::KeyValue *util::fgd::ClassDefinition::FindInput(const Data &fgdData,const std::string &name) const {return FindKeyValue(fgdData,KeyValueType::Input,name);}
const util::fgd::KeyValue *util::fgd::ClassDefinition::FindOutput(const Data &fgdData,const std::string &name) const {return FindKeyValue(fgdData,KeyValueType::Output,name);}

static util::MarkupFile::ResultCode read_arguments(util::MarkupFile &mf,std::vector<std::string> &arguments)
{
	std::string str {};
	util::MarkupFile::ResultCode r;
	if((r=mf.ReadUntil(ustring::WHITESPACE +'(',str)) != util::MarkupFile::ResultCode::Ok)
		return r;
	auto token = '\0';
	if((r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
		return r;
	mf.IncrementFilePos();
	while(token != ')')
	{
		str = {};

		if((r=mf.ReadNextString(str,"(),")) != util::MarkupFile::ResultCode::Ok || (r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
			return r;
		mf.IncrementFilePos();
		arguments.push_back(str);
	}
	return r;
}

static util::MarkupFile::ResultCode read_function(util::MarkupFile &mf)
{
	std::string str {};
	util::MarkupFile::ResultCode r {};
	auto offset = mf.GetDataStream()->GetOffset();
	if((r=mf.ReadUntil("=([:",str,false,true)) != util::MarkupFile::ResultCode::Ok || str.size() < 2)
		return r;
	auto token = str.back();
	str = str.substr(0,str.length() -1ull);
	if(token == '(')
	{
		std::vector<std::string> arguments {};
		read_arguments(mf,arguments);
	}
	else if(token == '=')
		mf.IncrementFilePos();
	else if(token == ':')
	{
		mf.GetDataStream()->SetOffset(offset);
		if((r=mf.ReadUntil(ustring::WHITESPACE,str,true)) != util::MarkupFile::ResultCode::Ok || str.size() < 2)
			return r;
	}
	return r;
}

static util::MarkupFile::ResultCode read_string_value(util::MarkupFile &mf,char &outToken,std::string &outStr)
{
	std::string str {};
	util::MarkupFile::ResultCode r {};
	if((r=mf.ReadUntil(ustring::WHITESPACE,str,true,false)) != util::MarkupFile::ResultCode::Ok)
		return r;
	str.clear();
	auto offset = mf.GetDataStream()->GetOffset();
	if((r=mf.ReadUntil(ustring::WHITESPACE +":+(=\"",str,false,true)) != util::MarkupFile::ResultCode::Ok)
		return r;
	outToken = str.back();
	auto name = str;
	switch(outToken)
	{
		case '\"':
		{
			mf.GetDataStream()->SetOffset(offset);
			name.clear();
			if((r=mf.ReadNextString(name)) != util::MarkupFile::ResultCode::Ok)
				return r;
			mf.IncrementFilePos();
			break;
		}
		case '+':
		{
			mf.IncrementFilePos();
			// TODO: Concatenate strings
			std::string subStr {};
			auto r = read_string_value(mf,outToken,subStr);
			outStr += subStr;
			break;
		}
		default:
			name.pop_back();
	}
	outStr = name;
	return r;
}

static util::fgd::PDataObject read_value(util::MarkupFile &mf,util::MarkupFile::ResultCode &resultCode)
{
	auto token = '\0';
	if((resultCode=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok || token == '[')
		return nullptr;
	std::string name {};
	if((resultCode=read_string_value(mf,token,name)) != util::MarkupFile::ResultCode::Ok)
		return nullptr;

	if(token == '\"')
	{
		if((resultCode=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
			return nullptr;
	}
	auto o = std::make_shared<util::fgd::DataObject>();
	o->name = name;
	if(ustring::WHITESPACE.find(token) != std::string::npos)
		return o;
	switch(token)
	{
		case '(':
			read_arguments(mf,o->arguments);
			break;
		case ':':
			return o;
		case '=':
			return o;
		case '+':
		{
			mf.IncrementFilePos();
			auto subValue = read_value(mf,resultCode);
			if(resultCode != util::MarkupFile::ResultCode::Ok || subValue == nullptr)
				return nullptr;
			o->name += subValue->name;
			break;
		}
		case '\n':
			break;
	}
	return o;
}

enum class State : uint8_t
{
	TopLevel = 0u,
	Parameters,
	Attributes,
	Children
};
static util::MarkupFile::ResultCode read_block(util::MarkupFile &mf,std::stack<util::fgd::PDataObject> &objectStack,State state=State::TopLevel)
{
	auto token = char{};
	for(;;)
	{
		util::MarkupFile::ResultCode r {};
		if((r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
			return r;
		switch(token)
		{
			case '/':
			{
				if((r=mf.ReadNextToken(token)) != util::MarkupFile::ResultCode::Ok)
				{
					if(token != '/')
						return util::MarkupFile::ResultCode::Error;
					mf.GetDataStream()->ReadLine();
					continue;
				}
				return util::MarkupFile::ResultCode::Error;
			}
			case '@':
			{
				auto o = read_value(mf,r);
				if(r != util::MarkupFile::ResultCode::Ok)
					return r;
				if(o == nullptr)
					throw std::runtime_error("Invalid value!");
				state = State::Parameters;
				objectStack.top()->children.push_back(o);
				break;
			}
			case '[':
			{
				mf.IncrementFilePos();
				objectStack.push(objectStack.top()->children.back());
				state = State::Children;
				break;
			}
			case ']':
				mf.IncrementFilePos();
				objectStack.pop();
				break;
			case '=':
			{
				mf.IncrementFilePos();
				auto o = read_value(mf,r);
				if(r != util::MarkupFile::ResultCode::Ok)
					return r;
				state = State::Attributes;
				if(o != nullptr)
					objectStack.top()->children.back()->attributes.push_back(o);
				else
					mf.DecrementFilePos(); // Last read character should be '['
				break;
			}
			default:
			{
				auto curState = state;
				if(token == ':')
				{
					curState = State::Attributes;
					mf.IncrementFilePos();
				}
				// Attribute
				auto o = read_value(mf,r);
				if(r != util::MarkupFile::ResultCode::Ok)
					return r;
				if(o == nullptr)
					throw std::runtime_error("Invalid attribute!");
				auto &children = objectStack.top()->children;
				switch(curState)
				{
					case State::Parameters:
						children.back()->parameters.push_back(o);
						break;
					case State::Attributes:
						children.back()->attributes.push_back(o);
						break;
					case State::Children:
						if(children.empty() == false && (ustring::compare(children.back()->name,"input",false) == true || ustring::compare(children.back()->name,"output",false) == true))
						{
							// Inputs and outputs have to be handled as special cases
							auto oSpecializer = std::make_shared<util::fgd::DataObject>();
							oSpecializer->name = children.back()->name;
							o->attributes.insert(o->attributes.begin(),oSpecializer);
							children.back() = o;
						}
						else
							children.push_back(o);
						break;
				}
			}
		}
	}
	return util::MarkupFile::ResultCode::Ok;
}

std::optional<util::fgd::Data> util::fgd::load_fgd(const std::string &fileName,const std::function<std::shared_ptr<VFilePtrInternal>(const std::string&)> &fileFactory,std::unordered_map<std::string,Data> &fgdCache)
{
	auto f = fileFactory(fileName);
	if(f == nullptr)
		return {};
	auto str = f->ReadString();
	DataStream ds {const_cast<void*>(reinterpret_cast<const void*>(str.data())),static_cast<uint32_t>(str.length())};
	util::MarkupFile mf{ds};
	std::stack<util::fgd::PDataObject> objectStack {};
	auto o = std::make_shared<util::fgd::DataObject>();
	objectStack.push(o);
	objectStack.top()->name = "root";
	read_block(mf,objectStack);

	// Convert raw data to FGD data structures
	util::fgd::Data data {};
	auto itMapSize = std::find_if(o->children.begin(),o->children.end(),[](const util::fgd::PDataObject &o) {
		return ustring::compare(o->name,"@mapsize",false);
	});
	if(itMapSize != o->children.end())
	{
		auto &args = (*itMapSize)->arguments;
		auto min = 0;
		auto max = 0;
		if(args.size() > 0u)
		{
			min = util::to_int(args.at(0u));
			if(args.size() > 1u)
				max = util::to_int(args.at(1u));
		}
		data.mapSize = {min,max};
	}
	for(auto &child : o->children)
	{
		if(ustring::compare(child->name,"@include",false) == true)
		{
			if(child->parameters.empty() == false)
			{
				auto &includeFile = child->parameters.front()->name;
				data.includes.push_back(includeFile);

				auto lIncludeFile = includeFile;
				ustring::to_lower(lIncludeFile);
				auto it = fgdCache.find(lIncludeFile);

				auto includeData = (it != fgdCache.end()) ? it->second : util::fgd::load_fgd(lIncludeFile,fileFactory,fgdCache);
				if(includeData.has_value())
				{
					// Merge data from included file with this file
					if(data.mapSize.first == 0u && data.mapSize.second == 0u)
						data.mapSize = includeData->mapSize;
					data.classDefinitions.reserve(data.classDefinitions.size() +includeData->classDefinitions.size());
					for(auto &pair : includeData->classDefinitions)
						data.classDefinitions.insert(pair);
				}
			}
			continue;
		}
		if(
			ustring::compare(child->name,"@BaseClass",false) == false &&
			ustring::compare(child->name,"@PointClass",false) == false &&
			ustring::compare(child->name,"@NPCClass",false) == false &&
			ustring::compare(child->name,"@SolidClass",false) == false &&
			ustring::compare(child->name,"@KeyFrameClass",false) == false &&
			ustring::compare(child->name,"@MoveClass",false) == false &&
			ustring::compare(child->name,"@FilterClass",false) == false
		)
			continue;
		auto classDef = std::make_shared<util::fgd::ClassDefinition>(data,*child);
		auto lname = classDef->GetName();
		ustring::to_lower(lname);
		data.classDefinitions.insert(std::make_pair(lname,classDef));
	}
	auto lFileName = fileName;
	ustring::to_lower(lFileName);
	fgdCache.insert(std::make_pair(lFileName,data));
	return data;
}

std::optional<util::fgd::Data> util::fgd::load_fgd(const std::string &fileName,const std::function<std::shared_ptr<VFilePtrInternal>(const std::string&)> &fileFactory)
{
	std::unordered_map<std::string,Data> fgdCache {};
	return load_fgd(fileName,fileFactory,fgdCache);
}

std::optional<util::fgd::Data> util::fgd::load_fgd(const std::string &fileName,std::unordered_map<std::string,Data> &fgdCache)
{
	return load_fgd(fileName,[](const std::string &fileName) {
		return FileManager::OpenFile(fileName.c_str(),"r");
	},fgdCache);
}

std::optional<util::fgd::Data> util::fgd::load_fgd(const std::string &fileName)
{
	std::unordered_map<std::string,Data> fgdCache {};
	return load_fgd(fileName,fgdCache);
}

static void print(const util::fgd::DataObject &o,const std::string &t="")
{
	std::cout<<o.name;
	if(o.arguments.empty() == false)
	{
		std::cout<<'(';
		auto bFirst = true;
		for(auto &arg : o.arguments)
		{
			if(bFirst == true)
				bFirst = false;
			else
				std::cout<<",";
			std::cout<<arg;
		}
		std::cout<<')';
	}
	if(o.attributes.empty() == false)
	{
		if(o.arguments.empty() == false)
			std::cout<<' '<<std::endl;
		std::cout<<'(';
		auto bFirst = true;
		for(auto &attr : o.attributes)
		{
			if(bFirst == true)
				bFirst = false;
			else
				std::cout<<",";
			print(*attr);
		}
		std::cout<<')';
	}
	for(auto &param : o.parameters)
		;

	//for(auto &child : o.children)
	//	print(*child,t +'\t');
}

#if 0
int main(int argc,char *argv[])
{
	auto fgData = util::fgd::load_fgd("hl.fgd");
	if(fgData.has_value() == false)
		return EXIT_FAILURE;

	auto itTriggerHurt = fgData->classDefinitions.find("xen_tree");
	if(itTriggerHurt != fgData->classDefinitions.end())
	{
		auto *pKeyValue = itTriggerHurt->second->FindKeyValue(*fgData,"filtername");
		std::cout<<"";
	}
	for(;;);
	return EXIT_SUCCESS;
}
#endif
