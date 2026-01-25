#include "pch.h"
#include "App/Logging/AppLogger.h"
#include "LaunchPlanner.h"
#include "Launcher/Version/Arguments.h"
#include "Launcher/Version/Library.h"

using namespace PCL_CPP::Core::Logging;

namespace PCL_CPP::Core::Launcher::Launch {

	LaunchPlanner::LaunchPlanner(const Version::VersionInfo &version, const LaunchContext &ctx)
		: _version(version), _ctx(ctx) {

		// Initialize features
		_features = _ctx.CustomFeatures;
		// Default features
		if (_features.find("is_demo_user") == _features.end()) _features["is_demo_user"] = false;
		if (_features.find("has_custom_resolution") == _features.end()) _features["has_custom_resolution"] = true;
	}

	ProcessStartInfo LaunchPlanner::Plan() {
		ProcessStartInfo info;
		info.Executable = _ctx.JavaPath;
		info.WorkingDirectory = _ctx.GameRoot; // Usually run in .minecraft

		// Classpath
		std::string cp = BuildClasspath();

		// JVM Args
		auto jvmArgs = BuildJvmArgs(cp);
		info.Arguments.insert(info.Arguments.end(), jvmArgs.begin(), jvmArgs.end());

		// Main Class
		info.Arguments.push_back(_version.MainClass);

		// Game Args
		auto gameArgs = BuildGameArgs();
		info.Arguments.insert(info.Arguments.end(), gameArgs.begin(), gameArgs.end());

		return info;
	}

	std::string LaunchPlanner::BuildClasspath() {
		std::string cp;
		std::string separator = ";"; // Windows separator

		auto librariesDir = _ctx.GameRoot / "libraries";

		// libraries
		const auto &libsJson = _version.RawData["libraries"];
		for (const auto &j : libsJson) {
			auto lib = Version::Library::Parse(j);

			// Only consider active libraries
			if (!lib.IsActive(_features)) continue;

			// Skip natives in classpath (they go to java.library.path)
			if (lib.IsNative()) continue;

			// Get path
			auto fileInfo = lib.GetApplicableFile(_features);
			std::filesystem::path libPath;

			if (fileInfo && !fileInfo->Path.empty()) {
				libPath = librariesDir / fileInfo->Path;
			} else {
				// Construct path from maven id
				libPath = librariesDir / MavenUtils::GetPath(lib.Name);
			}

			if (!cp.empty()) cp += separator;
			cp += libPath.string();
		}

		// Add client jar (Minecraft itself)
		std::filesystem::path clientJar;
		if (!_version.Jar.empty()) {
			clientJar = _ctx.GameRoot / "versions" / _version.Jar / (_version.Jar + ".jar");
		} else {
			clientJar = _version.RootPath / (_version.Jar + ".jar");
		}

		if (!cp.empty()) cp += separator;
		cp += clientJar.string();

		return cp;
	}

	std::map<std::string, std::string> LaunchPlanner::GetSubstitutions() {
		std::map<std::string, std::string> subs;

		// Auth
		subs["auth_player_name"] = _ctx.Auth.PlayerName;
		subs["auth_uuid"] = _ctx.Auth.Uuid;
		subs["auth_access_token"] = _ctx.Auth.AccessToken;
		subs["user_type"] = _ctx.Auth.UserType;

		// Version
		subs["version_name"] = _version.Id;
		subs["version_type"] = _version.Type;
		subs["assets_index_name"] = _version.AssetsIndex;

		// Paths
		subs["game_directory"] = _ctx.GameRoot.string();
		subs["assets_root"] = (_ctx.GameRoot / "assets").string();
		subs["natives_directory"] = _ctx.NativesDir.string();
		subs["launcher_name"] = "PCL2-CE-CPP";
		subs["launcher_version"] = "0.0.1";

		// Resolution
		subs["resolution_width"] = std::to_string(_ctx.Width);
		subs["resolution_height"] = std::to_string(_ctx.Height);

		return subs;
	}

	std::vector<std::string> LaunchPlanner::BuildJvmArgs(const std::string &classpath) {
		std::vector<std::string> args;

		// Base JVM args
		args.push_back("-Xmx" + std::to_string(_ctx.MaxMemoryMb) + "m");
		// args.push_back("-XX:HeapDumpPath=MojangTricksIntelDriversForPerformance_javaw.exe_minecraft.exe.heapdump");

		// Version-specific JVM args (if any)
		if (_version.RawData.contains("arguments") && _version.RawData["arguments"].contains("jvm")) {
			auto argParser = Version::Arguments::Parse(_version.RawData["arguments"]);
			auto subs = GetSubstitutions();
			subs["classpath"] = classpath; // Important!

			auto dynamicArgs = argParser.GetJvmArgs(subs, _features);
			args.insert(args.end(), dynamicArgs.begin(), dynamicArgs.end());
		} else {
			// Legacy handling (< 1.13 usually)
			// If arguments.jvm is missing, we must provide default legacy args
			args.push_back("-Djava.library.path=" + _ctx.NativesDir.string());
			args.push_back("-cp");
			args.push_back(classpath);
		}

		return args;
	}

	std::vector<std::string> LaunchPlanner::BuildGameArgs() {
		auto subs = GetSubstitutions();
		std::vector<std::string> args;

		if (_version.RawData.contains("arguments") && _version.RawData["arguments"].contains("game")) {
			// Modern (1.13+)
			auto argParser = Version::Arguments::Parse(_version.RawData["arguments"]);
			args = argParser.GetGameArgs(subs, _features);
		} else if (_version.RawData.contains("minecraftArguments")) {
			// Legacy (1.7.10 - 1.12.2)
			// "minecraftArguments": "--username ${auth_player_name} ..."
			std::string raw = _version.RawData["minecraftArguments"].get<std::string>();

			// Naive split by space
			std::stringstream ss(raw);
			std::string segment;
			while (std::getline(ss, segment, ' ')) {
				if (!segment.empty()) {
					for (const auto &[key, val] : subs) {
						std::string placeholder = "${" + key + "}";
						size_t pos = 0;
						while ((pos = segment.find(placeholder, pos)) != std::string::npos) {
							segment.replace(pos, placeholder.length(), val);
							pos += val.length();
						}
					}
					args.push_back(segment);
				}
			}
		}

		return args;
	}
}
