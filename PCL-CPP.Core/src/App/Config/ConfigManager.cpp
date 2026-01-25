#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "ConfigManager.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Config {

	ConfigManager &ConfigManager::GetInst() {
		static ConfigManager instance;
		return instance;
	}

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

	void ConfigManager::Init(const std::filesystem::path &configRoot) {
		{
			std::lock_guard<std::mutex> lock(m_managerMutex);
			m_configRoot = configRoot;

			// 确保目录存在
			if (!std::filesystem::exists(m_configRoot)) {
				std::filesystem::create_directories(m_configRoot);
			}

			// 处理 Template.json
			std::filesystem::path templatePath = m_configRoot / "Template.json";
			m_templateConfig = std::make_shared<ConfigContainer>();

			nlohmann::json templateData;
			if (std::filesystem::exists(templatePath)) {
				// 加载现有 Template
				m_templateConfig->Load(templatePath);
				templateData = m_templateConfig->GetJson();
			} else {
				// 不存在则使用硬编码默认值
				LOG_INFO("Template.json not found, creating from defaults.");
				templateData = GetHardcodedDefaults();
			}

			nlohmann::json hardcoded = GetHardcodedDefaults();
			bool dirty = false;

			// 递归补全
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
		LoadProfile("Default");
		LOG_INFO("ConfigManager initialized.");
	}

	void ConfigManager::LoadProfile(const std::string &profileName) {//?>????
		// 准备
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

		// 更新
		{
			std::lock_guard<std::mutex> lock(m_managerMutex);
			m_activeProfileName = profileName;

			// 复制 Template 作为基底
			nlohmann::json currentData = m_templateConfig->GetJson();

			// 合并差异
			if (profileExists) {
				RecursiveMerge(currentData, diffData);
			}

			// 更新 Active Profile
			m_activeProfile = std::make_shared<ConfigContainer>(currentData);
			LOG_INFO("Profile activated: {}", m_activeProfileName);
		}

		// 收尾
		if (!profileExists) {
			nlohmann::json initialDiff = nlohmann::json::object();
			initialDiff["ProfileName"] = profileName;
			ConfigContainer(initialDiff).Save(profilePath);
		}
	}

	void ConfigManager::SaveActiveProfile() {
		std::lock_guard<std::mutex> lock(m_managerMutex);
		if (!m_activeProfile || m_activeProfileName.empty()) return;

		auto profilePath = m_configRoot / (m_activeProfileName + ".json");
		nlohmann::json currentData = m_activeProfile->GetJson();
		nlohmann::json templateData = m_templateConfig->GetJson();

		// 计算差异
		nlohmann::json diffData = ComputeDiff(currentData, templateData);

		if (diffData.empty()) {
			LOG_TRACE("No changes detected for profile '{}'.", m_activeProfileName);
		} else {
			LOG_DEBUG("Saving diff for profile '{}'.", m_activeProfileName);
		}

		// 保存差异
		ConfigContainer(diffData).Save(profilePath);
	}

	std::shared_ptr<ConfigContainer> ConfigManager::GetTemplate() const {
		return m_templateConfig;
	}

	std::shared_ptr<ConfigContainer> ConfigManager::GetActiveProfile() const {
		return m_activeProfile;
	}

	// 递归补全
	void ConfigManager::RecursivePatch(nlohmann::json &target, const nlohmann::json &source) {
		if (!target.is_object() || !source.is_object()) return;

		for (auto &[key, val] : source.items()) {
			if (!target.contains(key)) {
				// 缺失键，补全
				target[key] = val;
			} else {
				// 键存在，检查类型
				if (target[key].type() != val.type()) {
					// 类型不匹配，强制覆盖（或者抛出警告）
					LOG_WARNING("Config type mismatch for key '{}', resetting to default.", key);
					target[key] = val;
				} else if (val.is_object()) {
					// 递归检查子对象
					RecursivePatch(target[key], val);
				}
			}
		}
	}

	void ConfigManager::RecursiveMerge(nlohmann::json &base, const nlohmann::json &diff) {
		if (!diff.is_object()) return;

		for (auto &[key, val] : diff.items()) {
			if (val.is_object() && base.contains(key) && base[key].is_object()) {
				RecursiveMerge(base[key], val);
			} else {
				base[key] = val; // 覆盖或新增
			}
		}
	}

	nlohmann::json ConfigManager::ComputeDiff(const nlohmann::json &target, const nlohmann::json &source) {
		nlohmann::json diff = nlohmann::json::object();
		if (!target.is_object() || !source.is_object()) return target; // 无法比较，直接返回 target

		for (auto &[key, val] : target.items()) {
			if (!source.contains(key)) {
				// Template 中没有的键，必须保存
				diff[key] = val;
			} else {
				// Template 中有
				if (val != source[key]) {
					// 值不同
					if (val.is_object() && source[key].is_object()) {
						// 递归比较
						nlohmann::json subDiff = ComputeDiff(val, source[key]);
						if (!subDiff.empty()) {
							diff[key] = subDiff;
						}
					} else {
						// 基础类型不同，保存
						diff[key] = val;
					}
				}
			}
		}
		return diff;
	}
}
