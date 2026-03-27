#include "mqtt_button.h"
#include "esphome/core/log.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_BUTTON

namespace esphome::mqtt {

static const char *const TAG = "mqtt.button";

using namespace esphome::button;

MQTTButtonComponent::MQTTButtonComponent(button::Button *button) : button_(button) {}

void MQTTButtonComponent::setup() {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto expose = this->get_homed_name();
    auto option = str_sprintf("\"%s\":{\"type\":\"button\"}", expose.c_str());
    mqtt::global_mqtt_client->get_homed_custom()->add_expose_with_option(expose, option);
    this->subscribe_json(this->get_command_topic_(), [this](const std::string &topic, JsonObject root) {
      auto name = this->get_homed_name().c_str();
      if ((root[name].is<bool>() && root[name] == true) || (root[name].is<const char*>() && root[name] == "true")) {
        this->button_->press();
      }
    });
  } else
  this->subscribe(this->get_command_topic_(), [this](const std::string &topic, const std::string &payload) {
    if (payload == "PRESS") {
      this->button_->press();
    } else {
      ESP_LOGW(TAG, "'%s': Received unknown status payload: %s", this->friendly_name_().c_str(), payload.c_str());
      this->status_momentary_warning("state", 5000);
    }
  });
}
void MQTTButtonComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT Button '%s': ", this->button_->get_name().c_str());
  LOG_MQTT_COMPONENT(false, true);
}

void MQTTButtonComponent::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  config.state_topic = false;
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    root[MQTT_PAYLOAD_PRESS] = "{\"" + name + "\": true}";
  }
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
}

MQTT_COMPONENT_TYPE(MQTTButtonComponent, "button")
const EntityBase *MQTTButtonComponent::get_entity() const { return this->button_; }

}  // namespace esphome::mqtt

#endif
#endif  // USE_MQTT
