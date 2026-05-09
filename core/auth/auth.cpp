#include "auth.h"
#include "../../vendor/keyauth/auth.hpp"
#include "keyauth_config.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#pragma comment(lib, "..\\vendor\\keyauth\\library_x64.lib")

using namespace KeyAuth;
namespace fs = std::filesystem;

static api KeyAuthApp(KEYAUTH_APP_NAME, KEYAUTH_OWNER_ID, KEYAUTH_VERSION, KEYAUTH_URL, "", KEYAUTH_DEBUG);

bool AuthManager::initialize() {
  try {
    KeyAuthApp.init();
    if (!KeyAuthApp.response.success) {
      status_message = KeyAuthApp.response.message;
      return false;
    }
  } catch (...) {
      status_message = "Exception during KeyAuth init";
      return false;
  }
  return true;
}

void AuthManager::saveCredentials(const std::string &user, const std::string &pass) {
    std::ofstream file("auth_data.txt");
    if (file.is_open()) {
        file << user << "\n" << pass;
        file.close();
    }
}

bool AuthManager::loadCredentials(std::string &user, std::string &pass) {
    std::ifstream file("auth_data.txt");
    if (file.is_open()) {
        std::getline(file, user);
        std::getline(file, pass);
        file.close();
        return !user.empty() && !pass.empty();
    }
    return false;
}

void AuthManager::loginFlow() {
    std::string user, pass, key;
    
    if (loadCredentials(user, pass)) {
        if (login(user, pass)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return;
        }
    }

    while (!authenticated) {
        system("cls");
        std::cout << "1. Login\n2. Register\n\n> ";
        int choice;
        std::cin >> choice;
        
        if (choice == 1) {
            system("cls");
            std::cout << "User: ";
            std::cin >> user;
            system("cls");
            std::cout << "Pass: ";
            std::cin >> pass;
            system("cls");
            
            if (login(user, pass)) {
                std::cout << "[+] Login successful!\n";
                saveCredentials(user, pass);
                std::this_thread::sleep_for(std::chrono::seconds(1));
                return;
            } else {
                std::cout << "[-] Login failed: " << status_message << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                exit(0);
            }
        } else if (choice == 2) {
            system("cls");
            std::cout << "User: ";
            std::cin >> user;
            system("cls");
            std::cout << "Pass: ";
            std::cin >> pass;
            system("cls");
            std::cout << "Key: ";
            std::cin >> key;
            system("cls");
            
            if (register_user(user, pass, key)) {
                std::cout << "[+] Registration successful! You can now login.\n";
                authenticated = false;
                std::this_thread::sleep_for(std::chrono::seconds(2));
            } else {
                std::cout << "[-] Registration failed: " << status_message << "\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                exit(0);
            }
        }
    }
}

bool AuthManager::isAuthenticated() { return authenticated; }

bool AuthManager::login(const std::string &user, const std::string &pass) {
  if (user.length() < 3 || pass.length() < 4) {
      status_message = "Invalid input length";
      return false;
  }
  KeyAuthApp.login(user, pass);
  authenticated = KeyAuthApp.response.success;
  status_message = KeyAuthApp.response.message;
  if (authenticated) {
    username = KeyAuthApp.user_data.username;
  }
  return authenticated;
}

bool AuthManager::register_user(const std::string &user, const std::string &pass, const std::string &key) {
  if (user.length() < 3 || pass.length() < 4) {
      status_message = "Invalid input length";
      return false;
  }
  KeyAuthApp.regstr(user, pass, key);
  authenticated = KeyAuthApp.response.success;
  status_message = KeyAuthApp.response.message;
  if (authenticated) {
    username = KeyAuthApp.user_data.username;
  }
  return authenticated;
}

std::string AuthManager::getUsername() { return username; }

std::string AuthManager::getStatus() { return status_message; }

std::vector<std::string> AuthManager::getSubscriptions() {
  std::vector<std::string> subs;
  for (const auto &sub : KeyAuthApp.user_data.subscriptions) {
    subs.push_back(sub.name);
  }
  return subs;
}

bool AuthManager::hasSubscription(const std::string &subName) {
  for (const auto &sub : KeyAuthApp.user_data.subscriptions) {
    if (sub.name == subName) return true;
  }
  return false;
}

bool AuthManager::validateSession() {
  return authenticated;
}
