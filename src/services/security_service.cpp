#include "security_service.h"
#include <cstdlib>
#include <ctime>
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <vector>

static const std::string b64_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const std::string &in) {
  std::string out;
  int val = 0, valb = -6;
  for (unsigned char c : in) {
    val = (val << 8) + c;
    valb += 8;
    while (valb >= 0) {
      out.push_back(b64_chars[(val >> valb) & 0x3F]);
      valb -= 6;
    }
  }
  if (valb > -6)
    out.push_back(b64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
  while (out.size() % 4)
    out.push_back('=');
  return out;
}

static std::string base64_decode(const std::string &in) {
  std::vector<int> T(256, -1);
  for (int i = 0; i < 64; i++)
    T[b64_chars[i]] = i;

  std::string out;
  int val = 0, valb = -8;
  for (unsigned char c : in) {
    if (T[c] == -1)
      continue;
    val = (val << 6) + T[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back((char)((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}

static std::string string_to_hex(const std::string &input) {
  static const char hex_digits[] = "0123456789abcdef";
  std::string output;
  output.reserve(input.length() * 2);
  for (unsigned char c : input) {
    output.push_back(hex_digits[c >> 4]);
    output.push_back(hex_digits[c & 15]);
  }
  return output;
}

std::string SecurityService::get_jwt_secret() {
  if (const char *env_secret = std::getenv("JWT_SECRET")) {
    return env_secret;
  }
  return "soc-super-secret-jwt-signing-key-usil-2026";
}

std::string SecurityService::get_webhook_secret() {
  if (const char *env_secret = std::getenv("WEBHOOK_SECRET")) {
    return env_secret;
  }
  return "lambda-soc-webhook-secret-token-key-2026";
}

std::string SecurityService::base64_url_encode(const std::string &data) {
  std::string b64 = base64_encode(data);
  std::string b64url;
  for (char c : b64) {
    if (c == '+')
      b64url.push_back('-');
    else if (c == '/')
      b64url.push_back('_');
    else if (c == '=')
      continue; // omit padding
    else
      b64url.push_back(c);
  }
  return b64url;
}

std::string SecurityService::base64_url_decode(const std::string &data) {
  std::string b64 = data;
  for (char &c : b64) {
    if (c == '-')
      c = '+';
    else if (c == '_')
      c = '/';
  }
  while (b64.size() % 4) {
    b64.push_back('=');
  }
  return base64_decode(b64);
}

std::string SecurityService::hmac_sha256(const std::string &data,
                                         const std::string &key) {
  unsigned int len = 32;
  unsigned char hash[32];
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
  HMAC(EVP_sha256(), key.c_str(), key.length(),
       reinterpret_cast<const unsigned char *>(data.c_str()), data.length(),
       hash, &len);
#else
  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);
  HMAC_Init_ex(&ctx, key.c_str(), key.length(), EVP_sha256(), NULL);
  HMAC_Update(&ctx, reinterpret_cast<const unsigned char *>(data.c_str()),
              data.length());
  HMAC_Final(&ctx, hash, &len);
  HMAC_CTX_cleanup(&ctx);
#endif

  return std::string(reinterpret_cast<char *>(hash), len);
}

std::string SecurityService::generate_jwt(const std::string &username,
                                          int expiry_seconds) {
  nlohmann::json header = {{"alg", "HS256"}, {"typ", "JWT"}};
  auto now = std::time(nullptr);
  nlohmann::json payload = {
      {"sub", username}, {"iat", now}, {"exp", now + expiry_seconds}};

  std::string part1 = base64_url_encode(header.dump());
  std::string part2 = base64_url_encode(payload.dump());
  std::string data = part1 + "." + part2;

  std::string sig = hmac_sha256(data, get_jwt_secret());
  std::string part3 = base64_url_encode(sig);

  return part1 + "." + part2 + "." + part3;
}

bool SecurityService::verify_jwt(const std::string &token,
                                 std::string &out_username) {
  auto dot1 = token.find('.');
  if (dot1 == std::string::npos)
    return false;
  auto dot2 = token.find('.', dot1 + 1);
  if (dot2 == std::string::npos)
    return false;

  std::string part1 = token.substr(0, dot1);
  std::string part2 = token.substr(dot1 + 1, dot2 - dot1 - 1);
  std::string part3 = token.substr(dot2 + 1);

  std::string data = part1 + "." + part2;
  std::string sig_expected = hmac_sha256(data, get_jwt_secret());
  std::string part3_expected = base64_url_encode(sig_expected);

  if (part3 != part3_expected) {
    return false;
  }

  try {
    std::string payload_str = base64_url_decode(part2);
    auto payload = nlohmann::json::parse(payload_str);
    if (!payload.contains("sub") || !payload.contains("exp")) {
      return false;
    }

    auto exp = payload["exp"].get<std::time_t>();
    if (std::time(nullptr) >= exp) {
      return false; // expired
    }

    out_username = payload["sub"].get<std::string>();
    return true;
  } catch (...) {
    return false;
  }
}

bool SecurityService::verify_webhook_signature(
    const std::string &payload, const std::string &signature_header) {
  if (signature_header.rfind("sha256=", 0) != 0) {
    return false;
  }
  std::string received_hex = signature_header.substr(7);
  std::string computed_sig = hmac_sha256(payload, get_webhook_secret());
  std::string computed_hex = string_to_hex(computed_sig);

  if (received_hex.length() != computed_hex.length()) {
    return false;
  }
  int diff = 0;
  for (size_t i = 0; i < received_hex.length(); ++i) {
    diff |= (received_hex[i] ^ computed_hex[i]);
  }
  return diff == 0;
}
