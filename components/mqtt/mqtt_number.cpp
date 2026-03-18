#include "mqtt_number.h"
#include "esphome/core/log.h"
#include "esphome/core/progmem.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_NUMBER

namespace esphome::mqtt {

static const char *const TAG = "mqtt.number";

using namespace esphome::number;

// Number mode MQTT strings indexed by NumberMode enum: AUTO(0) is skipped, BOX(1), SLIDER(2)
PROGMEM_STRING_TABLE(NumberMqttModeStrings, "", "box", "slider");

MQTTNumberComponent::MQTTNumberComponent(Number *number) : number_(number) {}

void MQTTNumberComponent::setup() {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto expose = this->get_homed_name();
    auto option = str_sprintf("\"%s\":{\"type\": \"number\", \"min\": %f, \"max\":  %f, \"step\": %f",
      expose.c_str(), this->number_->traits.get_min_value(), this->number_->traits.get_max_value(), this->number_->traits.get_step());
    const auto unit_of_measurement = this->number_->traits.get_unit_of_measurement_ref();
    if (!unit_of_measurement.empty())
      option += str_sprintf(", \"unit\": \"%s\"", unit_of_measurement.c_str());
    option += "}";
    mqtt::global_mqtt_client->get_homed_custom()->add_expose_with_option(expose, option);
    this->subscribe_json(this->get_command_topic_(), [this](const std::string &topic, JsonObject root) {
      auto name = this->get_homed_name().c_str();
      if (root[name].is<float>()) {
        float value = root[name];
        auto call = this->number_->make_call();
        call.set_value(value);
        call.perform();
      }
    });
  } else
  this->subscribe(this->get_command_topic_(), [this](const std::string &topic, const std::string &state) {
    auto val = parse_number<float>(state);
    if (!val.has_value()) {
      ESP_LOGW(TAG, "Can't convert '%s' to number!", state.c_str());
      return;
    }
    auto call = this->number_->make_call();
    call.set_value(*val);
    call.perform();
  });
  this->number_->add_on_state_callback([this](float state) { this->publish_state(state); });
}

void MQTTNumberComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT Number '%s':", this->number_->get_name().c_str());
  LOG_MQTT_COMPONENT(true, false);
}

MQTT_COMPONENT_TYPE(MQTTNumberComponent, "number")
const EntityBase *MQTTNumberComponent::get_entity() const { return this->number_; }

void MQTTNumberComponent::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  const auto &traits = number_->traits;
  // https://www.home-assistant.io/integrations/number.mqtt/
  // NOLINTNEXTLINE(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  root[MQTT_MIN] = traits.get_min_value();
  root[MQTT_MAX] = traits.get_max_value();
  root[MQTT_STEP] = traits.get_step();
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  const auto unit_of_measurement = this->number_->traits.get_unit_of_measurement_ref();
  if (!unit_of_measurement.empty()) {
    root[MQTT_UNIT_OF_MEASUREMENT] = unit_of_measurement;
  }
  const auto mode = this->number_->traits.get_mode();
  if (mode != NUMBER_MODE_AUTO) {
    root[MQTT_MODE] =
        NumberMqttModeStrings::get_progmem_str(static_cast<uint8_t>(mode), static_cast<uint8_t>(NUMBER_MODE_BOX));
  }
  const auto device_class = this->number_->traits.get_device_class_ref();
  if (!device_class.empty()) {
    root[MQTT_DEVICE_CLASS] = device_class;
  }
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    root[MQTT_COMMAND_TEMPLATE] = "{\"" + name + "\":{{ value }}}";
    root[MQTT_VALUE_TEMPLATE] = "{{ value_json." + name + " }}";
  }
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

  config.command_topic = true;
}
bool MQTTNumberComponent::send_initial_state() {
  if (this->number_->has_state()) {
    return this->publish_state(this->number_->state);
  } else {
    return true;
  }
}
bool MQTTNumberComponent::publish_state(float value) {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    return this->publish(this->get_state_topic_(), "{\"" + name + "\": \"" +
      std::to_string(value) + "\"}");
  }
  char topic_buf[MQTT_DEFAULT_TOPIC_MAX_LEN];
  char buffer[64];
  size_t len = buf_append_printf(buffer, sizeof(buffer), 0, "%f", value);
  return this->publish(this->get_state_topic_to_(topic_buf), buffer, len);
}

}  // namespace esphome::mqtt

#endif
#endif  // USE_MQTT
