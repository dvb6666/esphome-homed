#include "mqtt_switch.h"
#include "esphome/core/log.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_SWITCH

namespace esphome::mqtt {

static const char *const TAG = "mqtt.switch";

using namespace esphome::switch_;

MQTTSwitchComponent::MQTTSwitchComponent(switch_::Switch *a_switch) : switch_(a_switch) {}

void MQTTSwitchComponent::setup() {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto expose = this->get_homed_name();
    auto option = str_sprintf("\"%s\":{\"type\":\"toggle\"}", expose.c_str());
    mqtt::global_mqtt_client->get_homed_custom()->add_expose_with_option(expose, option);
    this->subscribe_json(this->get_command_topic_(), [this](const std::string &topic, JsonObject root) {
      auto name = this->get_homed_name().c_str();
      if (root[name].is<const char*>()) {
        switch (parse_on_off(root[name], "true", "false")) {
          case PARSE_ON:
            this->switch_->turn_on();
            break;
          case PARSE_OFF:
            this->switch_->turn_off();
            break;
          case PARSE_TOGGLE:
            this->switch_->toggle();
            break;
          case PARSE_NONE:
          default:
            ESP_LOGW(TAG, "'%s': Received unknown status payload: %s", this->friendly_name_().c_str(), name);
            this->status_momentary_warning("state", 5000);
            break;
        }
      }
    });
  } else
  this->subscribe(this->get_command_topic_(), [this](const std::string &topic, const std::string &payload) {
    switch (parse_on_off(payload.c_str())) {
      case PARSE_ON:
        this->switch_->turn_on();
        break;
      case PARSE_OFF:
        this->switch_->turn_off();
        break;
      case PARSE_TOGGLE:
        this->switch_->toggle();
        break;
      case PARSE_NONE:
      default:
        ESP_LOGW(TAG, "'%s': Received unknown status payload: %s", this->friendly_name_().c_str(), payload.c_str());
        this->status_momentary_warning("state", 5000);
        break;
    }
  });
  this->switch_->add_on_state_callback(
      [this](bool enabled) { this->defer("send", [this, enabled]() { this->publish_state(enabled); }); });
}
void MQTTSwitchComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT Switch '%s': ", this->switch_->get_name().c_str());
  LOG_MQTT_COMPONENT(true, true);
}

MQTT_COMPONENT_TYPE(MQTTSwitchComponent, "switch")
const EntityBase *MQTTSwitchComponent::get_entity() const { return this->switch_; }
void MQTTSwitchComponent::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  if (this->switch_->assumed_state()) {
    root[MQTT_OPTIMISTIC] = true;
  }
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    root[MQTT_VALUE_TEMPLATE] = "{{ value_json." + name + " | is_defined }}";
    root[MQTT_STATE_ON] = "true";
    root[MQTT_STATE_OFF] = "false";
    root[MQTT_PAYLOAD_ON] = "{\"" + name + "\": \"true\"}";
    root[MQTT_PAYLOAD_OFF] = "{\"" + name + "\": \"false\"}";
  }
}
bool MQTTSwitchComponent::send_initial_state() { return this->publish_state(this->switch_->state); }

bool MQTTSwitchComponent::publish_state(bool state) {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    return this->publish(this->get_state_topic_(), "{\"" + name + "\": \"" +
      std::string(state ? "true" : "false") + "\"}");
  }
  char topic_buf[MQTT_DEFAULT_TOPIC_MAX_LEN];
  const char *state_s = state ? "ON" : "OFF";
  return this->publish(this->get_state_topic_to_(topic_buf), state_s);
}

}  // namespace esphome::mqtt

#endif
#endif  // USE_MQTT
