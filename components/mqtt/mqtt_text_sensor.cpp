#include "mqtt_text_sensor.h"
#include "esphome/core/log.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_TEXT_SENSOR

namespace esphome::mqtt {

static const char *const TAG = "mqtt.text_sensor";

using namespace esphome::text_sensor;

MQTTTextSensor::MQTTTextSensor(TextSensor *sensor) : sensor_(sensor) {}
void MQTTTextSensor::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  const auto device_class = this->sensor_->get_device_class_ref();
  if (!device_class.empty()) {
    root[MQTT_DEVICE_CLASS] = device_class;
  }
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    root[MQTT_VALUE_TEMPLATE] = "{{ value_json." + name + " }}";
  }
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)
  config.command_topic = false;
}
void MQTTTextSensor::setup() {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto expose = this->get_homed_name();
    auto option = str_sprintf("\"%s\":{\"type\":\"sensor\"", expose.c_str());
    const auto device_class = this->sensor_->get_device_class_ref();
    if (!device_class.empty())
      option += str_sprintf(",\"class\":\"%s\"", device_class.c_str());
    option += "}";
    mqtt::global_mqtt_client->get_homed_custom()->add_expose_with_option(expose, option);
  }
  this->sensor_->add_on_state_callback([this](const std::string &state) { this->publish_state(state); });
}

void MQTTTextSensor::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT Text Sensor '%s':", this->sensor_->get_name().c_str());
  LOG_MQTT_COMPONENT(true, false);
}

bool MQTTTextSensor::publish_state(const std::string &value) {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    return this->publish(this->get_state_topic_(), "{\"" + name + "\": \"" + value + "\"}");
  }
  char topic_buf[MQTT_DEFAULT_TOPIC_MAX_LEN];
  return this->publish(this->get_state_topic_to_(topic_buf), value.data(), value.size());
}
bool MQTTTextSensor::send_initial_state() {
  if (this->sensor_->has_state()) {
    return this->publish_state(this->sensor_->state);
  } else {
    return true;
  }
}
MQTT_COMPONENT_TYPE(MQTTTextSensor, "sensor")
const EntityBase *MQTTTextSensor::get_entity() const { return this->sensor_; }

}  // namespace esphome::mqtt

#endif
#endif  // USE_MQTT
