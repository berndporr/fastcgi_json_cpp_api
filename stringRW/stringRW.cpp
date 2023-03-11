#include "json/json.h"
#include <iostream>

void jwrite() {
  Json::Value root;
  Json::Value data;
  for(int i=0;i<10;i++) {
	  data[i] = i*7;
  }
  root["action"] = "run";
  root["data"] = data;

  Json::StreamWriterBuilder builder;
  const std::string json_file = Json::writeString(builder, root);
  std::cout << json_file << std::endl;
}

void jread() {
  const std::string rawJson = R"({"Age": 20, "Name": "colin"})";
  const auto rawJsonLength = static_cast<int>(rawJson.length());
  JSONCPP_STRING err;
  Json::Value root;

  Json::CharReaderBuilder builder;
  const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(rawJson.c_str(), rawJson.c_str() + rawJsonLength, &root,
		     &err)) {
	  std::cout << "error" << std::endl;
	  exit(-1);
  }
  const std::string name = root["Name"].asString();
  const int age = root["Age"].asInt();

  std::cout << name << std::endl;
  std::cout << age << std::endl;
}

int main() {
	jread();
	jwrite();
	return EXIT_SUCCESS;
}
