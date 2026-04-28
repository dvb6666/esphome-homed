#include <cinttypes>
#include "mqtt_sensor.h"
#include "esphome/core/log.h"

#include "mqtt_const.h"

#ifdef USE_MQTT
#ifdef USE_SENSOR

#ifdef USE_DEEP_SLEEP
#include "esphome/components/deep_sleep/deep_sleep_component.h"
#endif

namespace esphome::mqtt {

static const char *const TAG = "mqtt.sensor";

using namespace esphome::sensor;

MQTTSensorComponent::MQTTSensorComponent(Sensor *sensor) : sensor_(sensor) {}

void MQTTSensorComponent::setup() {
  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto expose = this->get_homed_name();
    auto option = str_sprintf("\"%s\":{\"type\":\"sensor\"", expose.c_str());
    char dc_buf[MAX_DEVICE_CLASS_LENGTH];
    const char *dc = this->get_entity()->get_device_class_to(dc_buf);
    if (dc[0] != '\0')
      option += str_sprintf(",\"class\":\"%s\"", dc);
    if (this->sensor_->get_state_class() != STATE_CLASS_NONE)
      option += str_sprintf(",\"state\":\"%s\"", LOG_STR_ARG(state_class_to_string(this->sensor_->get_state_class())));
    const auto unit_of_measurement = this->sensor_->get_unit_of_measurement_ref();
    if (!unit_of_measurement.empty())
      option += str_sprintf(",\"unit\":\"%s\"", unit_of_measurement.c_str());
    if (this->sensor_->get_accuracy_decimals() > 0)
      option += str_sprintf(",\"round\":%d", this->sensor_->get_accuracy_decimals());
    option += "}";
    mqtt::global_mqtt_client->get_homed_custom()->add_expose_with_option(expose, option);
  }
  this->sensor_->add_on_state_callback([this](float state) { this->publish_state(state); });
}

void MQTTSensorComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "MQTT Sensor '%s':", this->sensor_->get_name().c_str());
  if (this->get_expire_after() > 0) {
    ESP_LOGCONFIG(TAG, "  Expire After: %" PRIu32 "s", this->get_expire_after() / 1000);
  }
  LOG_MQTT_COMPONENT(true, false);
}

MQTT_COMPONENT_TYPE(MQTTSensorComponent, "sensor")
const EntityBase *MQTTSensorComponent::get_entity() const { return this->sensor_; }

uint32_t MQTTSensorComponent::get_expire_after() const {
  if (this->expire_after_.has_value())
    return *this->expire_after_;
  return 0;
}
void MQTTSensorComponent::set_expire_after(uint32_t expire_after) { this->expire_after_ = expire_after; }
void MQTTSensorComponent::disable_expire_after() { this->expire_after_ = 0; }

void MQTTSensorComponent::send_discovery(JsonObject root, mqtt::SendDiscoveryConfig &config) {
  // NOLINTBEGIN(clang-analyzer-cplusplus.NewDeleteLeaks) false positive with ArduinoJson
  if (this->sensor_->has_accuracy_decimals()) {
    root[MQTT_SUGGESTED_DISPLAY_PRECISION] = this->sensor_->get_accuracy_decimals();
  }

  const auto unit_of_measurement = this->sensor_->get_unit_of_measurement_ref();
  if (!unit_of_measurement.empty()) {
    root[MQTT_UNIT_OF_MEASUREMENT] = unit_of_measurement;
  }
  // NOLINTEND(clang-analyzer-cplusplus.NewDeleteLeaks)

  if (this->get_expire_after() > 0)
    root[MQTT_EXPIRE_AFTER] = this->get_expire_after() / 1000;

  if (this->sensor_->get_force_update())
    root[MQTT_FORCE_UPDATE] = true;

  if (this->sensor_->get_state_class() != STATE_CLASS_NONE) {
#ifdef USE_STORE_LOG_STR_IN_FLASH
    root[MQTT_STATE_CLASS] = (const __FlashStringHelper *) state_class_to_string(this->sensor_->get_state_class());
#else
    root[MQTT_STATE_CLASS] = LOG_STR_ARG(state_class_to_string(this->sensor_->get_state_class()));
#endif
  }

  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    root[MQTT_VALUE_TEMPLATE] = "{{ value_json." + name + " }}";
  }
  config.command_topic = false;
}
bool MQTTSensorComponent::send_initial_state() {
  if (this->sensor_->has_state()) {
    return this->publish_state(this->sensor_->state);
  } else {
    return true;
  }
}
bool MQTTSensorComponent::publish_state(float value) {
  char topic_buf[MQTT_DEFAULT_TOPIC_MAX_LEN];
  if (mqtt::global_mqtt_client->is_publish_nan_as_none() && std::isnan(value))
    return this->publish(this->get_state_topic_to_(topic_buf), "None", 4);
  int8_t accuracy = this->sensor_->get_accuracy_decimals();
  char buf[VALUE_ACCURACY_MAX_LEN];
  size_t len = value_accuracy_to_buf(buf, value, accuracy);

  if (mqtt::global_mqtt_client->get_homed_custom()) {
    auto name = this->get_homed_name();
    return this->publish(this->get_state_topic_(), "{\"" + name + "\": \"" + buf + "\"}");
  }

  return this->publish(this->get_state_topic_to_(topic_buf), buf, len);
}

}  // namespace esphome::mqtt

#endif
#endif  // USE_MQTT
