#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "Library.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Version {

	/**
	 * @brief 解析 JSON 对象中的文件信息
	 * @param j JSON 数据
	 * @return 解析出的 FileInfo 对象，失败则返回 std::nullopt
	 */
	static std::optional<FileInfo> ParseFileInfo(const nlohmann::json &j) {
		if (!j.is_object()) return std::nullopt;
		FileInfo info;
		info.Path = j.value("path", "");
		info.Sha1 = j.value("sha1", "");
		info.Size = j.value("size", 0);
		info.Url = j.value("url", "");
		return info;
	}

	/**
	 * @brief 从 JSON 对象解析 Library
	 * @param j JSON 数据
	 * @return 解析出的 Library 对象
	 */
	Library Library::Parse(const nlohmann::json &j) {
		Library lib;
		lib.Name = j.value("name", "");

		// 解析下载信息 (downloads)
		if (j.contains("downloads")) {
			const auto &dl = j["downloads"];
			if (dl.contains("artifact")) {
				lib.Artifact = ParseFileInfo(dl["artifact"]);
			}
			if (dl.contains("classifiers")) {
				for (auto &[key, val] : dl["classifiers"].items()) {
					auto info = ParseFileInfo(val);
					if (info) lib.Classifiers[key] = *info;
				}
			}
		}

		// 解析 Native 映射 (natives)
		if (j.contains("natives")) {
			for (auto &[os, classifier] : j["natives"].items()) {
				lib.Natives[os] = classifier.get<std::string>();
			}
		}

		// 解析提取规则 (extract)
		if (j.contains("extract")) {
			ExtractRule ex;
			if (j["extract"].contains("exclude")) {
				for (const auto &item : j["extract"]["exclude"]) {
					ex.Exclude.push_back(item.get<std::string>());
				}
			}
			lib.Extract = ex;
		}

		// 解析启用规则 (rules)
		if (j.contains("rules")) {
			for (const auto &r : j["rules"]) {
				lib.Rules.push_back(Rule::Parse(r));
			}
		}

		return lib;
	}

	/**
	 * @brief 检查该库在当前环境下是否应被激活
	 * @param features 当前启用的功能开关
	 * @return 如果应激活则返回 true
	 */
	bool Library::IsActive(const std::map<std::string, bool> &features) const {
		if (Rules.empty()) return true;

		bool isAllowed = false;

		for (const auto &rule : Rules) {
			if (rule.IsMatch(features)) {
				isAllowed = (rule.Action == Rule::ActionType::Allow);
			}
		}
		return isAllowed;
	}

	/**
	 * @brief 检查该库是否为 Native 库
	 * @return 如果是 Native 库则返回 true
	 */
	bool Library::IsNative() const {
		return Natives.count("windows") > 0;
	}

	/**
	 * @brief 获取适用于当前环境的文件信息
	 * @param features 当前启用的功能开关
	 * @return 适用的文件信息，如果不适用则返回 std::nullopt
	 */
	std::optional<FileInfo> Library::GetApplicableFile(const std::map<std::string, bool> &features) const {
		if (!IsActive(features)) return std::nullopt;

		if (IsNative()) {
			std::string classifierKey = Natives.at("windows");

			// 处理占位符替换，例如 ${arch}
			size_t pos = classifierKey.find("${arch}");
			if (pos != std::string::npos) {
				std::string bitness = (SystemInfo::GetArch() == "x86") ? "32" : "64";
				classifierKey.replace(pos, 7, bitness);
			}

			auto it = Classifiers.find(classifierKey);
			if (it != Classifiers.end()) {
				return it->second;
			} else {
				return std::nullopt;
			}
		} else {
			if (Artifact.has_value()) return Artifact.value();
			return std::nullopt;
		}
	}
}
