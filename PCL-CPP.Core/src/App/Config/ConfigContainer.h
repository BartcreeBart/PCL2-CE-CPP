#pragma once
#include "pch.h"
#include <filesystem>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include "App/Logging/AppLogger.h"

namespace PCL_CPP::Core::Config {
	class ConfigContainer {
		public:
		ConfigContainer() = default;
		explicit ConfigContainer(const nlohmann::json &data) : m_data(data) { }

		// 加载配置文件
		void Load(const std::filesystem::path &path);

		// 保存配置文件
		void Save(const std::filesystem::path &path) const;

		// 获取 JSON 对象
		nlohmann::json GetJson() const;

		// 设置 JSON 对象
		void SetJson(const nlohmann::json &data);

		// 获取值
		template <typename T>
		T Get(const std::string &key, const T &defaultValue) const {
			std::shared_lock lock(m_mutex);
			try {
				// 使用 json_pointer 访问
				auto ptr = nlohmann::json::json_pointer("/" + key);
				if (m_data.contains(ptr)) {
					return m_data[ptr].get<T>();
				}
			} catch (const std::exception &e) {
				LOG_WARNING("Config Get failed for key '{}': {}. Using default.", key, e.what());
			} catch (...) {
				LOG_WARNING("Config Get failed for key '{}': Unknown error. Using default.", key);// 突发奇想
			}
			return defaultValue;
		}

		// 设置值
		template <typename T>
		void Set(const std::string &key, const T &value) {
			std::unique_lock lock(m_mutex);
			auto ptr = nlohmann::json::json_pointer("/" + key);

			if constexpr (std::is_enum_v<T>) {
				// 如果是枚举，自动转换为底层类型 (int/uint)
				m_data[ptr] = static_cast<typename std::underlying_type<T>::type>(value);
			} else {
				m_data[ptr] = value;
			}
		}

		private:
		mutable std::shared_mutex m_mutex;
		nlohmann::json m_data = nlohmann::json::object();
	};
}
