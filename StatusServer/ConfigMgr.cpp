#include "ConfigMgr.h"
#include <boost/filesystem.hpp>
#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


ConfigMgr::ConfigMgr() 
{
	//获取当前的工作目录
	boost::filesystem::path current_path = boost::filesystem::current_path();
	//构建完整的config.ini路径
	boost::filesystem::path config_path = current_path / "config.ini";
	std::cout << "Config path: " << config_path << std::endl;

	// 使用Boost.PropertyTree来读取INI文件  
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(),pt);

	// 遍历INI文件中的所有section  
	for (const auto& section_pair : pt)
	{
		const std::string& section_name = section_pair.first;
		const boost::property_tree::ptree& section_tree = section_pair.second;

		// 对于每个section，遍历其所有的key-value对 
		std::map<std::string, std::string> section_config;
		for (const auto& k_v_pair : section_tree) {
			const std::string& key = k_v_pair.first;
			const std::string& value = k_v_pair.second.get_value<std::string>();
			section_config[key] = value;

		}
		SectionInfo sectionInfo;
		sectionInfo._section_datas = section_config;
		_config_map[section_name] = sectionInfo;
	}

	for (const auto& section_entry : _config_map) {
		const std::string& section_name = section_entry.first;
		SectionInfo section_config = section_entry.second;
		std::cout << "[" << section_name << "]" << std::endl;
		for (const auto& key_value_pair : section_config._section_datas) {
			std::cout << key_value_pair.first << "=" << key_value_pair.second << std::endl;
		}
	}

}
