#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "ConfigManager.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Config {

	/**
	 * @brief 获取配置管理器单例实例
	 * @return ConfigManager 实例引用
	 */
	ConfigManager &ConfigManager::GetInst() {
		static ConfigManager instance;
		return instance;
	}

	/**
	 * @brief 获取硬编码的默认配置（版本 0）
	 * @return 默认 JSON 配置引用
	 */
	const nlohmann::json &ConfigManager::GetHardcodedDefaults() {
		static const nlohmann::json defaults = {
			{"Version", 0},
			{"ProfileName", "Template Profile"},
			{"Description", "Standard configuration template"},
			{"General", {
				{"Language", "zh-CN"}
			}}
		};
		return defaults;
	}

	/**
	 * @brief 初始化配置管理器
	 * @param configRoot 配置文件的根目录
	 */
	void ConfigManager::Init(const std::filesystem::path &configRoot) {
		{
			std::lock_guard<std::mutex> lock(m_managerMutex);
			m_configRoot = configRoot;

			// 确保配置根目录存在
			if (!std::filesystem::exists(m_configRoot)) {
				std::filesystem::create_directories(m_configRoot);
			}

			// 处理 Template.json (配置模板文件)
			std::filesystem::path templatePath = m_configRoot / "Template.json";
			m_templateConfig = std::make_shared<ConfigContainer>();

			nlohmann::json templateData;
			if (std::filesystem::exists(templatePath)) {
				// 加载现有的模板文件
				m_templateConfig->Load(templatePath);
				templateData = m_templateConfig->GetJson();
			} else {
				// 如果不存在，则使用硬编码默认值创建
				LOG_INFO("Template.json not found, creating from defaults.");
				templateData = GetHardcodedDefaults();
			}

			nlohmann::json hardcoded = GetHardcodedDefaults();
			bool dirty = false;

			// 递归补全模板中缺失的默认项
			nlohmann::json patchedData = templateData;
			RecursivePatch(patchedData, hardcoded);

			if (patchedData != templateData) {
				LOG_INFO("Template.json patched with new defaults.");
				templateData = patchedData;
				dirty = true;
			}

			m_templateConfig->SetJson(templateData);
			if (dirty || !std::filesystem::exists(templatePath)) {
				m_templateConfig->Save(templatePath);
			}
		}
		// 默认加载名为 "Default" 的配置 Profile
		LoadProfile("Default");
		LOG_INFO("ConfigManager initialized.");
	}

	/**
	 * @brief 加载指定的配置 Profile
	 * @param profileName Profile 名称（例如 "Default"）
	 */
	void ConfigManager::LoadProfile(const std::string &profileName) {
		auto profilePath = m_configRoot / (profileName + ".json");
		bool profileExists = false;
		nlohmann::json diffData;

		if (std::filesystem::exists(profilePath)) {
			profileExists = true;
			ConfigContainer tempContainer;
			tempContainer.Load(profilePath);
			diffData = tempContainer.GetJson();
		} else {
			LOG_WARNING("Profile {} not found, using template defaults.", profileName);
		}

		{
			std::lock_guard<std::mutex> lock(m_managerMutex);
			m_activeProfileName = profileName;

			// 复制 Template 模板数据作为基底
			nlohmann::json currentData = m_templateConfig->GetJson();

			// 如果 Profile 文件存在，则将其中的差异项合并到模板数据中
			if (profileExists) {
				RecursiveMerge(currentData, diffData);
			}

			// 更新当前激活的 Profile 容器
			m_activeProfile = std::make_shared<ConfigContainer>(currentData);
			LOG_INFO("Profile activated: {}", m_activeProfileName);
		}

		// 如果 Profile 文件原本不存在，则创建一个仅包含名称的空差异文件
		if (!profileExists) {
			nlohmann::json initialDiff = nlohmann::json::object();
			initialDiff["ProfileName"] = profileName;
			ConfigContainer(initialDiff).Save(profilePath);
		}
	}

	/**
	 * @brief 保存当前激活的配置 Profile (仅保存相对于模板的差异)
	 */
	void ConfigManager::SaveActiveProfile() {
		std::lock_guard<std::mutex> lock(m_managerMutex);
		if (!m_activeProfile || m_activeProfileName.empty()) return;

		auto profilePath = m_configRoot / (m_activeProfileName + ".json");
		nlohmann::json currentData = m_activeProfile->GetJson();
		nlohmann::json templateData = m_templateConfig->GetJson();

		// 计算当前数据相对于模板数据的差异
		nlohmann::json diffData = ComputeDiff(currentData, templateData);

		if (diffData.empty()) {
			LOG_TRACE("No changes detected for profile '{}'.", m_activeProfileName);
		} else {
			LOG_DEBUG("Saving diff for profile '{}'.", m_activeProfileName);
		}

		// 将差异保存到对应的 JSON 文件中
		ConfigContainer(diffData).Save(profilePath);
	}

	std::shared_ptr<ConfigContainer> ConfigManager::GetTemplate() const {
		return m_templateConfig;
	}

	std::shared_ptr<ConfigContainer> ConfigManager::GetActiveProfile() const {
		return m_activeProfile;
	}

	/**
	 * @brief 递归检查并补全配置项
	 * @param target 目标配置 JSON（将被修改）
	 * @param source 来源配置 JSON
	 */
	void ConfigManager::RecursivePatch(nlohmann::json &target, const nlohmann::json &source) {
		if (!target.is_object() || !source.is_object()) return;

		for (auto &[key, val] : source.items()) {
			if (!target.contains(key)) {
				// 缺失键，补全默认值
				target[key] = val;
			} else {
				// 键存在，检查类型一致性
				if (target[key].type() != val.type()) {
					LOG_WARNING("Config type mismatch for key '{}', resetting to default.", key);
					target[key] = val;
				} else if (val.is_object()) {
					// 递归检查子对象
					RecursivePatch(target[key], val);
				}
			}
		}
	}

	/**
	 * @brief 递归合并差异项到基础配置中
	 * @param base 基础 JSON 对象（将被修改）
	 * @param diff 包含差异的 JSON 对象
	 */
	void ConfigManager::RecursiveMerge(nlohmann::json &base, const nlohmann::json &diff) {
		if (!diff.is_object()) return;

		for (auto &[key, val] : diff.items()) {
			if (val.is_object() && base.contains(key) && base[key].is_object()) {
				RecursiveMerge(base[key], val);
			} else {
				base[key] = val; // 覆盖或新增项
			}
		}
	}

	/**
	 * @brief 递归计算两个 JSON 对象的差异
	 * @param target 目标 JSON 对象
	 * @param source 来源 JSON 对象
	 * @return 包含差异部分的 JSON 对象
	 */
	nlohmann::json ConfigManager::ComputeDiff(const nlohmann::json &target, const nlohmann::json &source) {
		nlohmann::json diff = nlohmann::json::object();
		if (!target.is_object() || !source.is_object()) return target;

		for (auto &[key, val] : target.items()) {
			if (!source.contains(key)) {
				// 模板中不存在的键，必须作为差异保存
				diff[key] = val;
			} else {
				if (val != source[key]) {
					// 值不同
					if (val.is_object() && source[key].is_object()) {
						// 递归比较子对象
						nlohmann::json subDiff = ComputeDiff(val, source[key]);
						if (!subDiff.empty()) {
							diff[key] = subDiff;
						}
					} else {
						// 基础类型值不同，保存差异
						diff[key] = val;
					}
				}
			}
		}
		return diff;
	}
}
