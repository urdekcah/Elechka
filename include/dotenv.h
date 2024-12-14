#ifndef __ELECHKA_DOTENV_H__
#define __ELECHKA_DOTENV_H__

#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
  #include <Windows.h>
#elif defined(__linux__)
  #include <limits.h>
  #include <unistd.h>
#else
  #error "Unsupported platform"
#endif

class env_loader {
public:
  enum class variable_source {
    any,
    cli_args,
    env_file,
    system_env
  };

private:
  struct env_entry {
    std::string value;
    variable_source source;
  };
  
  std::map<std::string, std::vector<env_entry>> env_storage;

  static std::string clean_whitespace(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
  }

  static std::string remove_quotes(const std::string& text) {
    std::string cleaned = clean_whitespace(text);
    if (cleaned.length() >= 2) {
      if ((cleaned.front() == '"' && cleaned.back() == '"') ||
        (cleaned.front() == '\'' && cleaned.back() == '\'')) {
        return cleaned.substr(1, cleaned.length() - 2);
      }
    }
    return cleaned;
  }

  void process_env_line(const std::string& line, variable_source src) {
    if (line.empty() || line[0] == '#') return;

    auto equals_pos = line.find('=');
    if (equals_pos == std::string::npos) {
      auto key = clean_whitespace(line);
      if (!key.empty()) {
        env_storage[key].push_back({std::string(), src});
      }
      return;
    }

    auto key = clean_whitespace(line.substr(0, equals_pos));
    auto raw_value = line.substr(equals_pos + 1);
    auto value = remove_quotes(raw_value);
    
    if (!key.empty()) {
      env_storage[key].push_back({value, src});
    }
  }

  void load_cli_arguments(int argc, const char** argv) {
    for (int i = 1; i < argc; ++i) {
      std::string arg = argv[i];
      if (arg.substr(0, 2) == "--") {
        process_env_line(arg.substr(2), variable_source::cli_args);
      }
    }
  }

  void load_env_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file) return;

    std::string line;
    while (std::getline(file, line)) {
      process_env_line(line, variable_source::env_file);
    }
  }

  static bool get_system_env(const std::string& name, std::string& value) {
#ifdef _WIN32
    char* buf = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buf, &size, name.c_str()) == 0 && buf != nullptr) {
      value = buf;
      free(buf);
      return true;
    }
#else
    if (const char* val = std::getenv(name.c_str())) {
      value = val;
      return true;
    }
#endif
    return false;
  }

public:
  env_loader(
    int argc = 0, 
    const char** argv = nullptr,
    const std::vector<std::string>& env_paths = {
      get_current_dir(".env"),
      get_executable_dir(".env")
    }
  ) {
    if (argc > 0 && argv != nullptr) {
      load_cli_arguments(argc, argv);
    }

    for (const auto& path : env_paths) {
      if (!path.empty()) {
        load_env_file(path);
      }
    }
  }

  bool has_variable(const std::string& name, variable_source src = variable_source::any) const {
    auto it = env_storage.find(name);
    if (it != env_storage.end()) {
      if (src == variable_source::any) return true;
      
      for (const auto& entry : it->second) {
        if (entry.source == src) return true;
      }
    }

    if (src == variable_source::any || src == variable_source::system_env) {
      std::string temp;
      return get_system_env(name, temp);
    }

    return false;
  }

  std::string get_value(
    const std::string& name, 
    const std::string& default_value = "", 
    variable_source src = variable_source::any
  ) const {
    auto it = env_storage.find(name);
    if (it != env_storage.end()) {
      if (src == variable_source::any) {
        for (const auto& entry : it->second) {
          if (entry.source == variable_source::cli_args) 
            return entry.value;
        }
        
        for (const auto& entry : it->second) {
          if (entry.source == variable_source::env_file) 
            return entry.value;
        }
      } else {
        for (const auto& entry : it->second) {
          if (entry.source == src) return entry.value;
        }
      }
    }

    if (src == variable_source::any || src == variable_source::system_env) {
      std::string sys_value;
      if (get_system_env(name, sys_value)) {
        return remove_quotes(sys_value);
      }
    }

    return default_value;
  }

  std::string operator[](const std::string& name) const {
    return get_value(name);
  }

  static std::string get_executable_path() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
#elif defined(__linux__)
    char path[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", path, PATH_MAX);
    return std::string(path, (count > 0) ? count : 0);
#else
    return "";
#endif
  }

  static std::string get_executable_dir(const std::string& relative_path = ".env") {
    std::string exe_path = get_executable_path();
    size_t last_sep = exe_path.find_last_of("/\\");
    
    if (last_sep == std::string::npos) {
      return relative_path;
    }
    
    return exe_path.substr(0, last_sep + 1) + relative_path;
  }

  static std::string get_current_dir(const std::string& relative_path = ".env") {
    return relative_path;
  }
};

#endif // __ELECHKA_DOTENV_H__