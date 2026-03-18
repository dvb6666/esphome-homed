# `ESPHome` component for [HOMEd Custon](https://wiki.homed.dev/custom/) integration.

To use this repository you should confugure it inside your yaml-configuration:
```yaml
mqtt:
  broker: !secret mqtt_host
  username: !secret mqtt_user
  password: !secret mqtt_pass
  log_topic: null
  discover_ip: false
  # discovery: false
  homed_custom:
    # topic_prefix: homed
    # device_id: test-homed
    # discovery: true
    # cloud: true

external_components:
  - source: github://dvb6666/esphome-homed
    refresh: 1sec
```

**More configuration examples you can find in [examples](examples) folder.**
