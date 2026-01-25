#pragma once
#include "Launcher/Version/VersionLocator.h"
#include <filesystem>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace PCL_CPP::Core::Launcher::Launch {

	struct LaunchAuth {
		std::string PlayerName = "Steve";
		std::string Uuid = "00000000-0000-0000-0000-000000000000";
		std::string AccessToken = "00000000000000000000000000000000";
		std::string UserType = "Legacy"; // Legacy, Mojang, MSA
	};

	struct LaunchContext {
		// Java
		std::filesystem::path JavaPath = "javaw.exe";
		int MaxMemoryMb = 2048;
		int MinMemoryMb = 512;

		// Auth
		LaunchAuth Auth;

		// Window
		int Width = 854;
		int Height = 480;
		bool Fullscreen = false;

		// Paths
		std::filesystem::path GameRoot; // .minecraft directory
		std::filesystem::path NativesDir; // Where natives are extracted

		// Features override 
		std::map<std::string, bool> CustomFeatures;
	};

	struct ProcessStartInfo {
		std::filesystem::path Executable;
		std::vector<std::string> Arguments;
		std::filesystem::path WorkingDirectory;

		// get full command line string
		std::string ToCommandLine() const {
			std::string cmd = Executable.string();
			for (const auto &arg : Arguments) {
				cmd += " ";
				if (arg.find(' ') != std::string::npos) {
					cmd += "\"" + arg + "\"";
				} else {
					cmd += arg;
				}
			}
			return cmd;
		}
	};

	class MavenUtils {
		public:
		// "com.google.guava:guava:31.0" -> "com/google/guava/guava/31.0/guava-31.0.jar"
		static std::filesystem::path GetPath(const std::string &mavenId, const std::string &extension = "jar", const std::string &classifier = "") {
			std::vector<std::string> parts;
			std::stringstream ss(mavenId);
			std::string item;
			while (std::getline(ss, item, ':')) {
				parts.push_back(item);
			}

			if (parts.size() < 3) return {};

			std::string group = parts[0];
			std::string artifact = parts[1];
			std::string version = parts[2];

			std::replace(group.begin(), group.end(), '.', '/');

			std::string filename = artifact + "-" + version;
			if (!classifier.empty()) {
				filename += "-" + classifier;
			}
			filename += "." + extension;

			return std::filesystem::path(group) / artifact / version / filename;
		}
	};

	class LaunchPlanner {
		public:
		LaunchPlanner(const Version::VersionInfo &version, const LaunchContext &ctx);

		ProcessStartInfo Plan();

		private:
		Version::VersionInfo _version;
		LaunchContext _ctx;
		std::map<std::string, bool> _features; // Effective features

		std::string BuildClasspath();
		std::vector<std::string> BuildJvmArgs(const std::string &classpath);
		std::vector<std::string> BuildGameArgs();

		std::map<std::string, std::string> GetSubstitutions();
	};
}
