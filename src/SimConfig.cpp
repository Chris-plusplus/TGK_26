#include <SimConfig.h>
#include <archimedes/Mmath.h>

SimConfig simConfig = [] -> SimConfig {
	auto json = nlohmann::json::parse(std::ifstream("simConfig.json"));

	json["maxAngleJerk"] = glm::radians(json["maxAngleJerk"].get<f32>());
	json["maxAngleChange"] = glm::radians(json["maxAngleChange"].get<f32>());

	return json.get<SimConfig>();
}();