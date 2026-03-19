#pragma once

#include "esphome.h"
#include "esphome/core/component.h"
#include "esphome/components/api/custom_api_device.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/climate/climate.h"
#include "esphome/components/climate/climate_mode.h"
#include "esphome/components/climate/climate_traits.h"
#include "messages.h"

namespace esphome {
namespace comfoair {

class ComfoAirComponent : public climate::Climate, public PollingComponent, public api::CustomAPIDevice, public uart::UARTDevice {
public:

  // Poll every 600ms
  ComfoAirComponent();

  void setup() override;
  climate::ClimateTraits traits() override;
  void control(const climate::ClimateCall &call) override;
  void dump_config() override;
  void update() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void control_set_curmode_speeds(float exhaust, float supply);
  void control_set_speeds(bool exhaust, bool supply, float off, float low, float mid, float high);
  void control_set_all_speeds(float supply_off, float supply_low, float supply_mid, float supply_high,
                               float exhaust_off, float exhaust_low, float exhaust_mid, float exhaust_high);
  void reset_filter();
  void set_uart_component(uart::UARTComponent *parent) { this->set_uart_parent(parent); }

protected:

  void set_level_(int level);
  void set_comfort_temperature_(float temperature);
  void write_command_(const uint8_t command, const uint8_t *command_data, uint8_t command_data_length);
  uint8_t comfoair_checksum_(const uint8_t *command_data, uint8_t length) const;
  optional<bool> check_byte_();
  void parse_data_();
  void get_fan_status_();
  void get_valve_status_();
  void get_error_status_();
  void get_bypass_control_status_();
  void get_temperature_();
  void get_sensor_data_();
  void get_ventilation_level_();
  void get_temperatures_();
  uint8_t get_uint8_t_(uint8_t start_index) const;
  uint16_t get_uint16_(uint8_t start_index) const;

  uint8_t data_[30];
  uint8_t data_index_{0};
  int8_t update_counter_{-3};

  uint8_t ventilation_levels_[8];

  uint8_t bootloader_version_[13]{0};
  uint8_t firmware_version_[13]{0};
  uint8_t connector_board_version_[14]{0};

public:
  sensor::Sensor *fan_supply_air_percentage{nullptr};
  sensor::Sensor *fan_exhaust_air_percentage{nullptr};
  sensor::Sensor *fan_speed_supply{nullptr};
  sensor::Sensor *fan_speed_exhaust{nullptr};
  sensor::Sensor *outside_air_temperature{nullptr};
  sensor::Sensor *supply_air_temperature{nullptr};
  sensor::Sensor *return_air_temperature{nullptr};
  sensor::Sensor *ventilation_level{nullptr};
  sensor::Sensor *exhaust_air_temperature{nullptr};
  sensor::Sensor *enthalpy_temperature{nullptr};
  sensor::Sensor *ewt_temperature{nullptr};
  sensor::Sensor *reheating_temperature{nullptr};
  sensor::Sensor *kitchen_hood_temperature{nullptr};
  sensor::Sensor *return_air_level{nullptr};
  sensor::Sensor *supply_air_level{nullptr};
  sensor::Sensor *bypass_factor{nullptr};
  sensor::Sensor *bypass_step{nullptr};
  sensor::Sensor *bypass_correction{nullptr};
  binary_sensor::BinarySensor *is_bypass_valve_open{nullptr};
  binary_sensor::BinarySensor *is_preheating{nullptr};
  binary_sensor::BinarySensor *is_summer_mode{nullptr};
  binary_sensor::BinarySensor *is_supply_fan_active{nullptr};
  binary_sensor::BinarySensor *is_filter_full{nullptr};

  void set_fan_supply_air_percentage(sensor::Sensor *fan_supply_air_percentage) { this->fan_supply_air_percentage = fan_supply_air_percentage; }
  void set_fan_exhaust_air_percentage(sensor::Sensor *fan_exhaust_air_percentage) { this->fan_exhaust_air_percentage = fan_exhaust_air_percentage; }
  void set_fan_speed_supply(sensor::Sensor *fan_speed_supply) { this->fan_speed_supply = fan_speed_supply; }
  void set_fan_speed_exhaust(sensor::Sensor *fan_speed_exhaust) { this->fan_speed_exhaust = fan_speed_exhaust; }
  void set_is_bypass_valve_open(binary_sensor::BinarySensor *is_bypass_valve_open) { this->is_bypass_valve_open = is_bypass_valve_open; }
  void set_is_preheating(binary_sensor::BinarySensor *is_preheating) { this->is_preheating = is_preheating; }
  void set_outside_air_temperature(sensor::Sensor *outside_air_temperature) { this->outside_air_temperature = outside_air_temperature; }
  void set_supply_air_temperature(sensor::Sensor *supply_air_temperature) { this->supply_air_temperature = supply_air_temperature; }
  void set_return_air_temperature(sensor::Sensor *return_air_temperature) { this->return_air_temperature = return_air_temperature; }
  void set_exhaust_air_temperature(sensor::Sensor *exhaust_air_temperature) { this->exhaust_air_temperature = exhaust_air_temperature; }
  void set_enthalpy_temperature(sensor::Sensor *enthalpy_temperature) { this->enthalpy_temperature = enthalpy_temperature; }
  void set_ewt_temperature(sensor::Sensor *ewt_temperature) { this->ewt_temperature = ewt_temperature; }
  void set_reheating_temperature(sensor::Sensor *reheating_temperature) { this->reheating_temperature = reheating_temperature; }
  void set_kitchen_hood_temperature(sensor::Sensor *kitchen_hood_temperature) { this->kitchen_hood_temperature = kitchen_hood_temperature; }
  void set_return_air_level(sensor::Sensor *return_air_level) { this->return_air_level = return_air_level; }
  void set_supply_air_level(sensor::Sensor *supply_air_level) { this->supply_air_level = supply_air_level; }
  void set_ventilation_level(sensor::Sensor *ventilation_level) { this->ventilation_level = ventilation_level; }
  void set_is_supply_fan_active(binary_sensor::BinarySensor *is_supply_fan_active) { this->is_supply_fan_active = is_supply_fan_active; }
  void set_is_filter_full(binary_sensor::BinarySensor *is_filter_full) { this->is_filter_full = is_filter_full; }
  void set_bypass_factor(sensor::Sensor *bypass_factor) { this->bypass_factor = bypass_factor; }
  void set_bypass_step(sensor::Sensor *bypass_step) { this->bypass_step = bypass_step; }
  void set_bypass_correction(sensor::Sensor *bypass_correction) { this->bypass_correction = bypass_correction; }
  void set_is_summer_mode(binary_sensor::BinarySensor *is_summer_mode) { this->is_summer_mode = is_summer_mode; }
};

}  // namespace comfoair
}  // namespace esphome
