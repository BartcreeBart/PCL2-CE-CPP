#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "ConfigContainer.h"
#include <fstream>

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Config {

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

	void ConfigContainer::Save(const std::filesystem::path &path) const {
		std::shared_lock lock(m_mutex);
		try {
			// 确保目录存在
			if (path.has_parent_path()) {
				std::filesystem::create_directories(path.parent_path());
			}

			std::ofstream file(path);
			if (file.is_open()) {
				file << m_data.dump(4); // 缩进
				LOG_DEBUG("Config saved successfully: {}", path.string());
			}
		} catch (const std::exception &e) {
			LOG_ERROR("Failed to save config file {}: {}", path.string(), e.what());
		}
	}

	nlohmann::json ConfigContainer::GetJson() const {
		std::shared_lock lock(m_mutex);
		return m_data;
	}

	void ConfigContainer::SetJson(const nlohmann::json &data) {
		std::unique_lock lock(m_mutex);
		m_data = data;
	}
}
