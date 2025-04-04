#pragma once

#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/preferences.h"
#include <vector>
#include <map>

namespace esphome {
namespace ble_key_manager {

struct BleKey {
  std::string name;
  std::string mac_address;
  bool enabled;
  bool require_button;
  uint32_t last_seen;  // timestamp dell'ultimo rilevamento
  
  BleKey() : name(""), mac_address(""), enabled(true), require_button(false), last_seen(0) {}
  
  BleKey(std::string name, std::string mac_address, bool enabled = true, bool require_button = false)
    : name(name), mac_address(mac_address), enabled(enabled), require_button(require_button), last_seen(0) {}
};

class BleKeyManager : public Component, public esp32_ble_tracker::ESPBTDeviceListener {
 public:
  BleKeyManager();
  
  // Metodi principali
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::LATE; }
  
  // Metodi per ESP BLE Tracker
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  
  // Metodi per gestione chiavi
  void add_key(const std::string &name, const std::string &mac_address, bool require_button = false);
  void remove_key(const std::string &mac_address);
  void set_key_status(const std::string &mac_address, bool enabled);
  void start_scan_mode(uint32_t duration);
  
  // Metodo per ottenere le chiavi (usato nell'esportazione)
  const std::vector<BleKey>& get_keys() const { return this->keys_; }
  
  // Metodo per eliminare tutte le chiavi (usato nell'importazione)
  void clear_all_keys();
  
  // Sensori e trigger
  void set_registered_keys_text_sensor(text_sensor::TextSensor *sensor) { this->registered_keys_sensor_ = sensor; }
  void set_last_detected_key_text_sensor(text_sensor::TextSensor *sensor) { this->last_detected_key_sensor_ = sensor; }
  void set_number_of_keys_sensor(sensor::Sensor *sensor) { this->num_keys_sensor_ = sensor; }
  void set_scan_interval(uint32_t interval) { this->scan_interval_ = interval; }
  void set_scan_duration(uint32_t duration) { this->scan_duration_ = duration; }
  void set_restore_from_flash(bool restore) { this->restore_from_flash_ = restore; }
  
  // Azioni
  void add_on_authorized_key_detected_callback(std::function<void()> callback);
  
 protected:
  // Storage delle chiavi
  std::vector<BleKey> keys_;
  ESPPreferenceObject flash_storage_;
  
  // Stato del componente
  bool scanning_{false};
  uint32_t scan_end_time_{0};
  uint32_t last_scan_time_{0};
  uint32_t scan_interval_{60000};  // Intervallo di scansione predefinito: 1 minuto
  uint32_t scan_duration_{5000};   // Durata della scansione predefinita: 5 secondi
  bool restore_from_flash_{true};
  bool manual_scan_mode_{false};
  uint32_t manual_scan_end_time_{0};
  
  // Sensori
  text_sensor::TextSensor *registered_keys_sensor_{nullptr};
  text_sensor::TextSensor *last_detected_key_sensor_{nullptr};
  sensor::Sensor *num_keys_sensor_{nullptr};
  
  // Azioni
  CallbackManager<void()> authorized_key_detected_callback_{};
  
  // Metodi interni
  void update_sensors_();
  void save_keys_to_flash_();
  void load_keys_from_flash_();
  std::string mac_to_string_(const uint64_t &mac);
  uint64_t string_to_mac_(const std::string &mac);
  void start_scan_();
  void stop_scan_();
};

// Classi per le azioni
template<typename... Ts> class AddKeyAction : public Action<Ts...> {
 public:
  AddKeyAction() {}
  
  void set_manager(BleKeyManager *manager) { this->manager_ = manager; }
  void set_name(std::function<std::string(Ts...)> name) { this->name_ = name; }
  void set_mac_address(std::function<std::string(Ts...)> mac_address) { this->mac_address_ = mac_address; }
  void set_require_button(std::function<bool(Ts...)> require_button) { this->require_button_ = require_button; }
  
  void play(Ts... x) override {
    this->manager_->add_key(this->name_(x...), this->mac_address_(x...), this->require_button_(x...));
  }
  
 protected:
  BleKeyManager *manager_;
  std::function<std::string(Ts...)> name_;
  std::function<std::string(Ts...)> mac_address_;
  std::function<bool(Ts...)> require_button_;
};

template<typename... Ts> class RemoveKeyAction : public Action<Ts...> {
 public:
  RemoveKeyAction() {}
  
  void set_manager(BleKeyManager *manager) { this->manager_ = manager; }
  void set_mac_address(std::function<std::string(Ts...)> mac_address) { this->mac_address_ = mac_address; }
  
  void play(Ts... x) override {
    this->manager_->remove_key(this->mac_address_(x...));
  }
  
 protected:
  BleKeyManager *manager_;
  std::function<std::string(Ts...)> mac_address_;
};

template<typename... Ts> class SetKeyStatusAction : public Action<Ts...> {
 public:
  SetKeyStatusAction() {}
  
  void set_manager(BleKeyManager *manager) { this->manager_ = manager; }
  void set_mac_address(std::function<std::string(Ts...)> mac_address) { this->mac_address_ = mac_address; }
  void set_enabled(std::function<bool(Ts...)> enabled) { this->enabled_ = enabled; }
  
  void play(Ts... x) override {
    this->manager_->set_key_status(this->mac_address_(x...), this->enabled_(x...));
  }
  
 protected:
  BleKeyManager *manager_;
  std::function<std::string(Ts...)> mac_address_;
  std::function<bool(Ts...)> enabled_;
};

template<typename... Ts> class StartScanModeAction : public Action<Ts...> {
 public:
  StartScanModeAction() {}
  
  void set_manager(BleKeyManager *manager) { this->manager_ = manager; }
  void set_duration(std::function<uint32_t(Ts...)> duration) { this->duration_ = duration; }
  
  void play(Ts... x) override {
    this->manager_->start_scan_mode(this->duration_(x...));
  }
  
 protected:
  BleKeyManager *manager_;
  std::function<uint32_t(Ts...)> duration_;
};

class BLEKeyDetectedTrigger : public Trigger<> {
 public:
  explicit BLEKeyDetectedTrigger(BleKeyManager *parent) {
    parent->add_on_authorized_key_detected_callback([this]() { this->trigger(); });
  }
};

}  // namespace ble_key_manager
}  // namespace esphome