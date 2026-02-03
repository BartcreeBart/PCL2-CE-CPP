#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <shared_mutex>
#include "App/Logging/AppLogger.h"

namespace PCL_CPP::Core::Config {
	/**
	 * @brief 配置容器类，用于管理和访问 JSON 格式的配置数据。
	 * 
	 * 该类提供线程安全的配置读取和写入操作，支持枚举类型的自动转换，
	 * 并通过 json_pointer 支持嵌套键值的访问。
	 */
	class ConfigContainer {
		public:
		/**
		 * @brief 默认构造函数
		 */
		ConfigContainer() = default;

		/**
		 * @brief 使用现有的 JSON 数据构造配置容器
		 * @param data JSON 数据
		 */
		explicit ConfigContainer(const nlohmann::json &data) : m_data(data) { }

		/**
		 * @brief 从指定路径加载配置文件
		 * @param path 配置文件路径
		 */
		void Load(const std::filesystem::path &path);

		/**
		 * @brief 将配置文件保存到指定路径
		 * @param path 配置文件保存路径
		 */
		void Save(const std::filesystem::path &path) const;

		/**
		 * @brief 获取原始 JSON 对象
		 * @return nlohmann::json 对象
		 */
		nlohmann::json GetJson() const;

		/**
		 * @brief 设置原始 JSON 对象
		 * @param data 要设置的 JSON 数据
		 */
		void SetJson(const nlohmann::json &data);

		/**
		 * @brief 获取配置项的值
		 * @tparam T 配置项的类型
		 * @param key 配置项的键（支持 json_pointer 路径）
		 * @param defaultValue 键不存在或获取失败时的默认值
		 * @return 配置项的值或默认值
		 */
		template <typename T>
		T Get(const std::string &key, const T &defaultValue) const {
			std::shared_lock lock(m_mutex);
			try {
				// 使用 json_pointer 访问
				auto ptr = nlohmann::json::json_pointer("/" + key);
				if (m_data.contains(ptr)) {
					if constexpr (std::is_enum_v<T>) {
						// 如果是枚举，自动从底层类型 (int/uint) 转换回来
						return static_cast<T>(m_data[ptr].get<typename std::underlying_type<T>::type>());
					} else {
						return m_data[ptr].get<T>();
					}
				}
			} catch (const std::exception &e) {
				LOG_WARNING("Config Get failed for key '{}': {}. Using default.", key, e.what());
			} catch (...) {
				LOG_WARNING("Config Get failed for key '{}': Unknown error. Using default.", key);
			}
			return defaultValue;
		}

		/**
		 * @brief 设置配置项的值
		 * @tparam T 配置项的类型
		 * @param key 配置项的键（支持 json_pointer 路径）
		 * @param value 要设置的值
		 */
		template <typename T>
		void Set(const std::string &key, const T &value) {
			std::unique_lock lock(m_mutex);
			try {
				auto ptr = nlohmann::json::json_pointer("/" + key);

				if constexpr (std::is_enum_v<T>) {
					// 如果是枚举，自动转换为底层类型 (int/uint)
					m_data[ptr] = static_cast<typename std::underlying_type<T>::type>(value);
				} else {
					m_data[ptr] = value;
				}
			} catch (const std::exception &e) {
				LOG_WARNING("Config Set failed for key '{}': {}.", key, e.what());
			} catch (...) {
				LOG_WARNING("Config Set failed for key '{}': Unknown error.", key);
			}
		}

		private:
		mutable std::shared_mutex m_mutex; ///< 用于保证线程安全的读写锁
		nlohmann::json m_data = nlohmann::json::object(); ///< 存储配置数据的 JSON 对象
	};
}
