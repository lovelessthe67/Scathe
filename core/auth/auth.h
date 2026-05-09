#pragma once
#include <memory>
#include <string>
#include <vector>


class SecureKeyAuth;

bool performAuthentication();
SecureKeyAuth *getAuthInstance();

class AuthManager {
public:
  static bool initialize();
  static bool isAuthenticated();
  static void loginFlow();
  static bool login(const std::string &user, const std::string &pass);
  static bool register_user(const std::string &user, const std::string &pass, const std::string &key);
  static std::string getUsername();
  static std::string getStatus();
  static std::vector<std::string> getSubscriptions();
  static bool hasSubscription(const std::string &subName);
  static bool validateSession();
  static void bypassAuth() {
    authenticated = true;
    username = "bypassed";
  }

  static void saveCredentials(const std::string &user, const std::string &pass);
  static bool loadCredentials(std::string &user, std::string &pass);

private:
  static inline bool authenticated = false;
  static inline std::string username = "";
  static inline std::string status_message = "";
};
