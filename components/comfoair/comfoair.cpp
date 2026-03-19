#include "comfoair.h"

namespace esphome {
namespace comfoair {

ComfoAirComponent::ComfoAirComponent() :
  Climate(),
  PollingComponent(600),
  UARTDevice() {}

void ComfoAirComponent::setup() {
  register_service(&ComfoAirComponent::control_set_speeds, "climate_set_speeds", {"exhaust_fan", "supply_fan", "off", "low", "mid", "high"});
  register_service(&ComfoAirComponent::control_set_all_speeds, "climate_set_all_speeds", {"supply_off", "supply_low", "supply_mid", "supply_high", "exhaust_off", "exhaust_low", "exhaust_mid", "exhaust_high"});
  register_service(&ComfoAirComponent::control_set_curmode_speeds, "climate_set_current_mode_speeds", {"exhaust", "supply"});
}

void ComfoAirComponent::control_set_curmode_speeds(float exhaust, float supply) {
  ESP_LOGI(TAG, "Setting speeds for level %i to: %.0f,%.0f", ventilation_level->state, exhaust, supply);
  uint8_t command_data[COMFOAIR_SET_VENTILATION_LEVEL_LENGTH] = {
      (ventilation_level->state==0x01) ? ventilation_levels_[0] : (uint8_t)exhaust,
      (ventilation_level->state==0x02) ? ventilation_levels_[2] : (uint8_t)exhaust,
      (ventilation_level->state==0x03) ? ventilation_levels_[4] : (uint8_t)exhaust,
      (ventilation_level->state==0x01) ? ventilation_levels_[1] : (uint8_t)supply,
      (ventilation_level->state==0x02) ? ventilation_levels_[3] : (uint8_t)supply,
      (ventilation_level->state==0x03) ? ventilation_levels_[5] : (uint8_t)supply,
      (ventilation_level->state==0x04) ? ventilation_levels_[6] : (uint8_t)exhaust,
      (ventilation_level->state==0x04) ? ventilation_levels_[7] : (uint8_t)supply,
      (uint8_t)0x00
  };
  write_command_(COMFOAIR_SET_VENTILATION_LEVEL_REQUEST, command_data, sizeof(command_data));
}

void ComfoAirComponent::control_set_speeds(bool exhaust, bool supply, float off, float low, float mid, float high) {
  ESP_LOGI(TAG, "Setting speeds to: %.0f,%.0f,%.0f,%.0f", off, low, mid, high);
  uint8_t command_data[COMFOAIR_SET_VENTILATION_LEVEL_LENGTH] = {
      !exhaust ? ventilation_levels_[0] : (uint8_t)off,
      !exhaust ? ventilation_levels_[2] : (uint8_t)low,
      !exhaust ? ventilation_levels_[4] : (uint8_t)mid,
      !supply ? ventilation_levels_[1] : (uint8_t)off,
      !supply ? ventilation_levels_[3] : (uint8_t)low,
      !supply ? ventilation_levels_[5] : (uint8_t)mid,
      !exhaust ? ventilation_levels_[6] : (uint8_t)high,
      !supply ? ventilation_levels_[7] : (uint8_t)high,
      (uint8_t)0x00
  };
  write_command_(COMFOAIR_SET_VENTILATION_LEVEL_REQUEST, command_data, sizeof(command_data));
}

void ComfoAirComponent::control_set_all_speeds(float supply_off, float supply_low, float supply_mid, float supply_high,
                                                float exhaust_off, float exhaust_low, float exhaust_mid, float exhaust_high) {
  ESP_LOGI(TAG, "Setting speeds for supply to: %.0f,%.0f,%.0f,%.0f; exhaust: %.0f,%.0f,%.0f,%.0f",
           supply_off, supply_low, supply_mid, supply_high, exhaust_off, exhaust_low, exhaust_mid, exhaust_high);
  uint8_t command_data[COMFOAIR_SET_VENTILATION_LEVEL_LENGTH] = {
      (uint8_t)exhaust_off,
      (uint8_t)exhaust_low,
      (uint8_t)exhaust_mid,
      (uint8_t)supply_off,
      (uint8_t)supply_low,
      (uint8_t)supply_mid,
      (uint8_t)exhaust_high,
      (uint8_t)supply_high,
      (uint8_t)0x00
  };
  write_command_(COMFOAIR_SET_VENTILATION_LEVEL_REQUEST, command_data, sizeof(command_data));
}

/// Return the traits of this controller.
climate::ClimateTraits ComfoAirComponent::traits() {
  auto traits = climate::ClimateTraits();
  traits.add_feature_flags(esphome::climate::CLIMATE_SUPPORTS_CURRENT_TEMPERATURE);
  traits.set_supported_modes({
    climate::CLIMATE_MODE_FAN_ONLY
  });
  traits.set_supported_presets({
      climate::CLIMATE_PRESET_HOME,
  });
  traits.set_visual_min_temperature(12);
  traits.set_visual_max_temperature(29);
  traits.set_supported_fan_modes({
    climate::CLIMATE_FAN_AUTO,
    climate::CLIMATE_FAN_LOW,
    climate::CLIMATE_FAN_MEDIUM,
    climate::CLIMATE_FAN_HIGH,
    climate::CLIMATE_FAN_OFF,
  });
  return traits;
}

/// Override control to change settings of the climate device.
void ComfoAirComponent::control(const climate::ClimateCall &call) {
  if (call.get_fan_mode().has_value()) {
    int level;

    this->fan_mode = *call.get_fan_mode();
    switch (this->fan_mode.value()) {
      case climate::CLIMATE_FAN_HIGH:
        level = 0x04;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        level = 0x03;
        break;
      case climate::CLIMATE_FAN_LOW:
        level = 0x02;
        break;
      case climate::CLIMATE_FAN_OFF:
        level = 0x01;
        break;
      case climate::CLIMATE_FAN_AUTO:
        level = 0x00;
        break;
      case climate::CLIMATE_FAN_ON:
      case climate::CLIMATE_FAN_MIDDLE:
      case climate::CLIMATE_FAN_DIFFUSE:
      default:
        level = -1;
        break;
    }

    if (level >= 0) {
      set_level_(level);
    }
  }

  if (call.get_target_temperature().has_value()) {
    this->target_temperature = *call.get_target_temperature();
    set_comfort_temperature_(this->target_temperature);
  }

  this->publish_state();
}

void ComfoAirComponent::dump_config() {
  uint8_t *p;
  ESP_LOGCONFIG(TAG, "ComfoAir:");
  p = bootloader_version_;
  ESP_LOGCONFIG(TAG, "  Bootloader %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
  p = firmware_version_;
  ESP_LOGCONFIG(TAG, "  Firmware %.10s v%0d.%02d b%2d", p + 3, *p, *(p + 1), *(p + 2));
  p = connector_board_version_;
  ESP_LOGCONFIG(TAG, "  Connector Board %.10s v%0d.%02d", p + 2, *p, *(p + 1));

  if (*(p + 12) != 0) {
    ESP_LOGCONFIG(TAG, "  CC-Ease v%0d.%02d", *(p + 12) >> 4, *(p + 12) & 0x0f);
  }
  if (*(p + 13) != 0) {
    ESP_LOGCONFIG(TAG, "  CC-Luxe v%0d.%02d", *(p + 13) >> 4, *(p + 13) & 0x0f);
  }
  this->check_uart_settings(9600);
}

void ComfoAirComponent::update() {
  switch (update_counter_) {
    case -3:
      this->write_command_(COMFOAIR_GET_BOOTLOADER_VERSION_REQUEST, nullptr, 0);
      break;
    case -2:
      this->write_command_(COMFOAIR_GET_FIRMWARE_VERSION_REQUEST, nullptr, 0);
      break;
    case -1:
      this->write_command_(COMFOAIR_GET_BOARD_VERSION_REQUEST, nullptr, 0);
      break;
    case 0:
      get_fan_status_();
      break;
    case 1:
      get_valve_status_();
      break;
    case 2:
      get_sensor_data_();
      break;
    case 3:
      get_ventilation_level_();
      break;
    case 4:
      get_temperatures_();
      break;
    case 5:
      get_error_status_();
      break;
    case 6:
      get_bypass_control_status_();
      break;
  }

  update_counter_++;
  if (update_counter_ > 6)
    update_counter_ = 0;
}

void ComfoAirComponent::loop() {
  while (this->available() != 0) {
    this->read_byte(&this->data_[this->data_index_]);
    auto check = this->check_byte_();
    if (!check.has_value()) {
      // finished
      if (this->data_[COMFOAIR_MSG_ACK_IDX] != COMFOAIR_MSG_ACK) {
        this->parse_data_();
      }
      this->data_index_ = 0;
    } else if (!*check) {
      // wrong data
      ESP_LOGV(TAG, "Byte %i of received data frame is invalid.", this->data_index_);
      this->data_index_ = 0;
    } else {
      // next byte
      this->data_index_++;
    }
  }
}

void ComfoAirComponent::reset_filter() {
  uint8_t reset_cmd[COMFOAIR_SET_RESET_LENGTH] = {0, 0, 0, 1};
  this->write_command_(COMFOAIR_SET_RESET_REQUEST, reset_cmd, sizeof(reset_cmd));
}

void ComfoAirComponent::set_level_(int level) {
  if (level < 0 || level > 4) {
    ESP_LOGI(TAG, "Ignoring invalid level request: %i", level);
    return;
  }

  ESP_LOGI(TAG, "Setting level to: %i", level);
  uint8_t command[COMFOAIR_SET_LEVEL_LENGTH] = {(uint8_t) level};
  this->write_command_(COMFOAIR_SET_LEVEL_REQUEST, command, sizeof(command));
}

void ComfoAirComponent::set_comfort_temperature_(float temperature) {
  if (temperature < 12.0f || temperature > 29.0f) {
    ESP_LOGI(TAG, "Ignoring invalid temperature request: %i", temperature);
    return;
  }

  ESP_LOGI(TAG, "Setting temperature to: %i", temperature);
  uint8_t command[COMFOAIR_SET_COMFORT_TEMPERATURE_LENGTH] = {(uint8_t) ((temperature + 20.0f) * 2.0f)};
  this->write_command_(COMFOAIR_SET_COMFORT_TEMPERATURE_REQUEST, command, sizeof(command));
}

void ComfoAirComponent::write_command_(const uint8_t command, const uint8_t *command_data, uint8_t command_data_length) {
  this->write_byte(COMFOAIR_MSG_PREFIX);
  this->write_byte(COMFOAIR_MSG_HEAD);
  this->write_byte(0x00);
  this->write_byte(command);
  this->write_byte(command_data_length);
  if (command_data_length > 0) {
    this->write_array(command_data, command_data_length);
    this->write_byte((command + command_data_length + comfoair_checksum_(command_data, command_data_length)) & 0xff);
  } else {
    this->write_byte(comfoair_checksum_(&command, 1));
  }
  this->write_byte(COMFOAIR_MSG_PREFIX);
  this->write_byte(COMFOAIR_MSG_TAIL);
  this->flush();
}

uint8_t ComfoAirComponent::comfoair_checksum_(const uint8_t *command_data, uint8_t length) const {
  uint8_t sum = 0;
  for (uint8_t i = 0; i < length; i++) {
    sum += command_data[i];
  }
  return sum + 0xad;
}

optional<bool> ComfoAirComponent::check_byte_() {
  uint8_t index = this->data_index_;
  uint8_t byte = this->data_[index];

  if (index == 0) {
    return byte == COMFOAIR_MSG_PREFIX;
  }

  if (index == 1) {
    if (byte == COMFOAIR_MSG_ACK) {
      return {};
    } else {
      return byte == COMFOAIR_MSG_HEAD;
    }
  }

  if (index == 2) {
    return byte == 0x00;
  }

  if (index < COMFOAIR_MSG_HEAD_LENGTH) {
    return true;
  }

  uint8_t data_length = this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX];

  if ((COMFOAIR_MSG_HEAD_LENGTH + data_length + COMFOAIR_MSG_TAIL_LENGTH) > sizeof(this->data_)) {
    ESP_LOGW(TAG, "ComfoAir message too large");
    return false;
  }

  if (index < COMFOAIR_MSG_HEAD_LENGTH + data_length) {
    // handle escaped 0x07 byte (sequence of two 0x07 0x07 taken as one)
    if (byte == COMFOAIR_MSG_PREFIX && this->data_[index-1] == COMFOAIR_MSG_PREFIX) {
        this->data_index_--;
    }
    return true;
  }

  if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length) {
    // checksum is without checksum bytes
    uint8_t checksum = comfoair_checksum_(this->data_ + 2, COMFOAIR_MSG_HEAD_LENGTH + data_length - 2);
    if (checksum != byte) {
      //ESP_LOGW(TAG, "%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X", this->data_[0], this->data_[1], this->data_[2], this->data_[3], this->data_[4], this->data_[5], this->data_[6], this->data_[7], this->data_[8], this->data_[9], this->data_[10]);
      ESP_LOGW(TAG, "ComfoAir Checksum doesn't match: 0x%02X!=0x%02X", byte, checksum);
      return false;
    }
    return true;
  }

  if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 1) {
    return byte == COMFOAIR_MSG_PREFIX;
  }

  if (index == COMFOAIR_MSG_HEAD_LENGTH + data_length + 2) {
    if (byte != COMFOAIR_MSG_TAIL) {
      return false;
    }
  }

  return {};
}

void ComfoAirComponent::parse_data_() {
  this->status_clear_warning();
  uint8_t *msg = &this->data_[COMFOAIR_MSG_HEAD_LENGTH];

  switch (this->data_[COMFOAIR_MSG_IDENTIFIER_IDX]) {
    case COMFOAIR_GET_BOOTLOADER_VERSION_RESPONSE:
      memcpy(bootloader_version_, msg, this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
      break;
    case COMFOAIR_GET_FIRMWARE_VERSION_RESPONSE:
      memcpy(firmware_version_, msg, this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
      break;
    case COMFOAIR_GET_BOARD_VERSION_RESPONSE:
      memcpy(connector_board_version_, msg, this->data_[COMFOAIR_MSG_DATA_LENGTH_IDX]);
      break;
    case COMFOAIR_GET_FAN_STATUS_RESPONSE: {
      if (this->fan_supply_air_percentage != nullptr) {
        this->fan_supply_air_percentage->publish_state(msg[0]);
      }
      if (this->fan_exhaust_air_percentage != nullptr) {
        this->fan_exhaust_air_percentage->publish_state(msg[1]);
      }
      if (this->fan_speed_supply != nullptr) {
        this->fan_speed_supply->publish_state(1875000.0f / this->get_uint16_(2));
      }
      if (this->fan_speed_exhaust != nullptr) {
        this->fan_speed_exhaust->publish_state(1875000.0f / this->get_uint16_(4));
      }
      break;
    }
    case COMFOAIR_GET_VALVE_STATUS_RESPONSE: {
      if (this->is_bypass_valve_open != nullptr) {
        this->is_bypass_valve_open->publish_state(msg[0] != 0);
      }
      if (this->is_preheating != nullptr) {
        this->is_preheating->publish_state(msg[1] != 0);
      }
      break;
    }
    case COMFOAIR_GET_BYPASS_CONTROL_RESPONSE: {
      if (this->bypass_factor != nullptr) {
        this->bypass_factor->publish_state(msg[2]);
      }
      if (this->bypass_step != nullptr) {
        this->bypass_step->publish_state(msg[3]);
      }
      if (this->bypass_correction != nullptr) {
        this->bypass_correction->publish_state(msg[4]);
      }
      if (this->is_summer_mode != nullptr) {
        this->is_summer_mode->publish_state(msg[6] != 0);
      }
      break;
    }
    case COMFOAIR_GET_TEMPERATURE_RESPONSE: {
      // T1 / outside air
      if (this->outside_air_temperature != nullptr) {
        this->outside_air_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
      }
      // T2 / supply air
      if (this->supply_air_temperature != nullptr) {
        this->supply_air_temperature->publish_state((float) msg[1] / 2.0f - 20.0f);
      }
      // T3 / return air
      if (this->return_air_temperature != nullptr) {
        this->return_air_temperature->publish_state((float) msg[2] / 2.0f - 20.0f);
      }
      // T4 / exhaust air
      if (this->exhaust_air_temperature != nullptr) {
        this->exhaust_air_temperature->publish_state((float) msg[3] / 2.0f - 20.0f);
      }
      break;
    }
    case COMFOAIR_GET_SENSOR_DATA_RESPONSE: {
      if (this->enthalpy_temperature != nullptr) {
        this->enthalpy_temperature->publish_state((float) msg[0] / 2.0f - 20.0f);
      }
      break;
    }
    case COMFOAIR_GET_VENTILATION_LEVEL_RESPONSE: {
      ESP_LOGD(TAG, "Level %02x", msg[8]);

      if (this->return_air_level != nullptr) {
        this->return_air_level->publish_state(msg[6]);
      }
      if (this->supply_air_level != nullptr) {
        this->supply_air_level->publish_state(msg[7]);
      }
      if (ventilation_level != nullptr) {
        ventilation_level->publish_state(msg[8] - 1);
      }

      // Fan Speed
      switch (msg[8]) {
        case 0x00:
          this->fan_mode = climate::CLIMATE_FAN_AUTO;
          this->mode = climate::CLIMATE_MODE_AUTO;
          break;
        case 0x01:
          this->fan_mode = climate::CLIMATE_FAN_OFF;
          this->mode = climate::CLIMATE_MODE_OFF;
          break;
        case 0x02:
          this->fan_mode = climate::CLIMATE_FAN_LOW;
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
        case 0x03:
          this->fan_mode = climate::CLIMATE_FAN_MEDIUM;
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
        case 0x04:
          this->fan_mode = climate::CLIMATE_FAN_HIGH;
          this->mode = climate::CLIMATE_MODE_FAN_ONLY;
          break;
      }

      this->publish_state();

      // Supply air fan active (1 = active / 0 = inactive)
      if (this->is_supply_fan_active != nullptr) {
        this->is_supply_fan_active->publish_state(msg[9] == 1);
      }

      // Record current speeds for resetting them if needed.
      if (msg[0]) ventilation_levels_[0] = msg[0];
      if (msg[3]) ventilation_levels_[1] = msg[3];
      if (msg[1]) ventilation_levels_[2] = msg[1];
      if (msg[4]) ventilation_levels_[3] = msg[4];
      if (msg[2]) ventilation_levels_[4] = msg[2];
      if (msg[5]) ventilation_levels_[5] = msg[5];
      if (msg[10]) ventilation_levels_[6] = msg[10];
      if (msg[11]) ventilation_levels_[7] = msg[11];

      break;
    }
    case COMFOAIR_GET_ERROR_STATE_RESPONSE: {
      if (this->is_filter_full != nullptr) {
        this->is_filter_full->publish_state(msg[8] != 0);
      }
      break;
    }
    case COMFOAIR_GET_TEMPERATURES_RESPONSE: {
      // comfort temperature
      this->target_temperature = (float) msg[0] / 2.0f - 20.0f;
      this->publish_state();

      // T1 / outside air
      if (this->outside_air_temperature != nullptr && msg[5] & 0x01) {
        this->outside_air_temperature->publish_state((float) msg[1] / 2.0f - 20.0f);
      }
      // T2 / supply air
      if (this->supply_air_temperature != nullptr && msg[5] & 0x02) {
        this->supply_air_temperature->publish_state((float) msg[2] / 2.0f - 20.0f);
      }
      // T3 / exhaust air
      if (this->return_air_temperature != nullptr && msg[5] & 0x04) {
        this->return_air_temperature->publish_state((float) msg[3] / 2.0f - 20.0f);
        this->current_temperature = (float) msg[3] / 2.0f - 20.0f;
      }
      // T4 / continued air
      if (this->exhaust_air_temperature != nullptr && msg[5] & 0x08) {
        this->exhaust_air_temperature->publish_state((float) msg[4] / 2.0f - 20.0f);
      }
      // EWT
      if (this->ewt_temperature != nullptr && msg[5] & 0x10) {
        this->ewt_temperature->publish_state((float) msg[6] / 2.0f - 20.0f);
      }
      // reheating
      if (this->reheating_temperature != nullptr && msg[5] & 0x20) {
        this->reheating_temperature->publish_state((float) msg[7] / 2.0f - 20.0f);
      }
      // kitchen hood
      if (this->kitchen_hood_temperature != nullptr && msg[5] & 0x40) {
        this->kitchen_hood_temperature->publish_state((float) msg[8] / 2.0f - 20.0f);
      }
      break;
    }
  }
}

void ComfoAirComponent::get_fan_status_() {
  if (this->fan_supply_air_percentage != nullptr ||
      this->fan_exhaust_air_percentage != nullptr ||
      this->fan_speed_supply != nullptr ||
      this->fan_speed_exhaust != nullptr) {
    ESP_LOGD(TAG, "getting fan status");
    this->write_command_(COMFOAIR_GET_FAN_STATUS_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_valve_status_() {
  if (this->is_bypass_valve_open != nullptr ||
      this->is_preheating != nullptr) {
    ESP_LOGD(TAG, "getting valve status");
    this->write_command_(COMFOAIR_GET_VALVE_STATUS_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_error_status_() {
  if (this->is_filter_full != nullptr) {
    ESP_LOGD(TAG, "getting error status");
    this->write_command_(COMFOAIR_GET_ERROR_STATE_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_bypass_control_status_() {
  if (this->bypass_factor != nullptr ||
      this->bypass_step != nullptr ||
      this->bypass_correction != nullptr ||
      this->is_summer_mode != nullptr) {
    ESP_LOGD(TAG, "getting bypass control");
    this->write_command_(COMFOAIR_GET_BYPASS_CONTROL_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_temperature_() {
  if (this->outside_air_temperature != nullptr ||
      this->supply_air_temperature != nullptr ||
      this->return_air_temperature != nullptr ||
      this->exhaust_air_temperature != nullptr) {
    ESP_LOGD(TAG, "getting temperature");
    this->write_command_(COMFOAIR_GET_TEMPERATURE_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_sensor_data_() {
  if (this->enthalpy_temperature != nullptr) {
    ESP_LOGD(TAG, "getting sensor data");
    this->write_command_(COMFOAIR_GET_SENSOR_DATA_REQUEST, nullptr, 0);
  }
}

void ComfoAirComponent::get_ventilation_level_() {
  ESP_LOGD(TAG, "getting ventilation level");
  this->write_command_(COMFOAIR_GET_VENTILATION_LEVEL_REQUEST, nullptr, 0);
}

void ComfoAirComponent::get_temperatures_() {
  ESP_LOGD(TAG, "getting temperatures");
  this->write_command_(COMFOAIR_GET_TEMPERATURES_REQUEST, nullptr, 0);
}

uint8_t ComfoAirComponent::get_uint8_t_(uint8_t start_index) const {
  return this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index];
}

uint16_t ComfoAirComponent::get_uint16_(uint8_t start_index) const {
  return (uint16_t(this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index + 1] | this->data_[COMFOAIR_MSG_HEAD_LENGTH + start_index] << 8));
}

}  // namespace comfoair
}  // namespace esphome
