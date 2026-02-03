#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "ConfigContainer.h"
#include <fstream>

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Config {

	/**
	 * @brief 从指定路径加载配置文件
	 * @param path 配置文件路径
	 */
	void ConfigContainer::Load(const std::filesystem::path &path) {
		std::unique_lock lock(m_mutex);
		LOG_DEBUG("Loading config from: {}", path.string());
		if (!std::filesystem::exists(path)) {
			LOG_WARNING("Config file not found: {}", path.string());
			m_data = nlohmann::json::object();
			return;
		}

		try {
			std::ifstream file(path);
			if (file.is_open()) {
				file >> m_data;
				LOG_TRACE("Config loaded successfully: {}", path.string());
			}
		} catch (const std::exception &e) {
			LOG_ERROR("Failed to parse config file {}: {}", path.string(), e.what());
			m_data = nlohmann::json::object(); // 解析失败重置为空
		}
	}

	/**
	 * @brief 将配置文件保存到指定路径
	 * @param path 配置文件保存路径
	 */
	void ConfigContainer::Save(const std::filesystem::path &path) const {
		std::shared_lock lock(m_mutex);
		try {
			// 确保目录存在
			if (path.has_parent_path()) {
				std::filesystem::create_directories(path.parent_path());
			}

			std::ofstream file(path);
			if (file.is_open()) {
				file << m_data.dump(4); // 使用 4 空格缩进
				LOG_DEBUG("Config saved successfully: {}", path.string());
			}
		} catch (const std::exception &e) {
			LOG_ERROR("Failed to save config file {}: {}", path.string(), e.what());
		}
	}

	/**
	 * @brief 获取原始 JSON 对象
	 * @return nlohmann::json 对象
	 */
	nlohmann::json ConfigContainer::GetJson() const {
		std::shared_lock lock(m_mutex);
		return m_data;
	}

	/**
	 * @brief 设置原始 JSON 对象
	 * @param data 要设置的 JSON 数据
	 */
	void ConfigContainer::SetJson(const nlohmann::json &data) {
		std::unique_lock lock(m_mutex);
		m_data = data;
	}
}
