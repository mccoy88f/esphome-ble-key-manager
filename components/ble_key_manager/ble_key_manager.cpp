#include "ble_key_manager.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <ArduinoJson.h>

namespace esphome {
namespace ble_key_manager {

static const char *TAG = "ble_key_manager";

BleKeyManager::BleKeyManager() {}

void BleKeyManager::setup() {
  ESP_LOGD(TAG, "Setting up BLE Key Manager...");
  
  // Inizializza la memoria flash
  this->flash_storage_ = global_preferences->make_preference<std::vector<BleKey>>(1919598706UL);
  
  // Carica le chiavi dalla memoria flash
  if (this->restore_from_flash_) {
    this->load_keys_from_flash_();
  }
  
  // Aggiorna i sensori
  this->update_sensors_();
}

void BleKeyManager::loop() {
  // Gestione della scansione BLE
  const uint32_t now = millis();
  
  // Gestione modalità di scansione manuale
  if (this->manual_scan_mode_ && now > this->manual_scan_end_time_) {
    this->manual_scan_mode_ = false;
    ESP_LOGD(TAG, "Modalità scansione manuale terminata");
  }
  
  // Avvio scansione periodica
  if (!this->scanning_ && !this->manual_scan_mode_ && 
      (now - this->last_scan_time_ > this->scan_interval_)) {
    this->start_scan_();
  }
  
  // Termine scansione
  if (this->scanning_ && now > this->scan_end_time_) {
    this->stop_scan_();
  }
}

bool BleKeyManager::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  // Converti MAC in formato stringa
  std::string mac = this->mac_to_string_(device.address_uint64());
  
  // Cerca il dispositivo nella lista delle chiavi
  for (auto &key : this->keys_) {
    if (key.mac_address == mac) {
      // Aggiorna timestamp
      key.last_seen = millis();
      
      // Se la chiave è abilitata, attiva il callback
      if (key.enabled) {
        ESP_LOGI(TAG, "Rilevata chiave autorizzata: %s (%s)", key.name.c_str(), mac.c_str());
        
        // Aggiorna il sensore dell'ultima chiave rilevata
        if (this->last_detected_key_sensor_ != nullptr) {
          this->last_detected_key_sensor_->publish_state(key.name);
        }
        
        // Attiva il callback se non è richiesto il pulsante o se siamo in modalità scansione manuale
        if (!key.require_button || this->manual_scan_mode_) {
          this->authorized_key_detected_callback_.call();
        }
        
        // Salva lo stato aggiornato
        this->save_keys_to_flash_();
        return true;
      }
    }
  }
  
  return false;
}

void BleKeyManager::add_key(const std::string &name, const std::string &mac_address, bool require_button) {
  // Verifica se la chiave esiste già
  for (auto &key : this->keys_) {
    if (key.mac_address == mac_address) {
      ESP_LOGI(TAG, "Aggiornamento chiave esistente: %s", mac_address.c_str());
      key.name = name;
      key.require_button = require_button;
      key.enabled = true;
      this->save_keys_to_flash_();
      this->update_sensors_();
      return;
    }
  }
  
  // Aggiungi nuova chiave
  ESP_LOGI(TAG, "Aggiunta nuova chiave: %s (%s)", name.c_str(), mac_address.c_str());
  this->keys_.push_back(BleKey(name, mac_address, true, require_button));
  this->save_keys_to_flash_();
  this->update_sensors_();
}

void BleKeyManager::remove_key(const std::string &mac_address) {
  for (auto it = this->keys_.begin(); it != this->keys_.end(); ++it) {
    if (it->mac_address == mac_address) {
      ESP_LOGI(TAG, "Rimozione chiave: %s (%s)", it->name.c_str(), mac_address.c_str());
      this->keys_.erase(it);
      this->save_keys_to_flash_();
      this->update_sensors_();
      return;
    }
  }
  
  ESP_LOGW(TAG, "Chiave non trovata: %s", mac_address.c_str());
}

void BleKeyManager::set_key_status(const std::string &mac_address, bool enabled) {
  for (auto &key : this->keys_) {
    if (key.mac_address == mac_address) {
      ESP_LOGI(TAG, "%s chiave: %s (%s)", 
              enabled ? "Attivazione" : "Disattivazione", key.name.c_str(), mac_address.c_str());
      key.enabled = enabled;
      this->save_keys_to_flash_();
      this->update_sensors_();
      return;
    }
  }
  
  ESP_LOGW(TAG, "Chiave non trovata: %s", mac_address.c_str());
}

void BleKeyManager::start_scan_mode(uint32_t duration) {
  ESP_LOGI(TAG, "Avvio modalità scansione manuale per %u ms", duration);
  this->manual_scan_mode_ = true;
  this->manual_scan_end_time_ = millis() + duration;
  
  // Forza l'avvio della scansione
  if (!this->scanning_) {
    this->start_scan_();
  }
}

void BleKeyManager::add_on_authorized_key_detected_callback(std::function<void()> callback) {
  this->authorized_key_detected_callback_.add(std::move(callback));
}

void BleKeyManager::update_sensors_() {
  if (this->num_keys_sensor_ != nullptr) {
    this->num_keys_sensor_->publish_state(this->keys_.size());
  }
  
  if (this->registered_keys_sensor_ != nullptr) {
    DynamicJsonDocument json_doc(4096);
    JsonArray json_keys = json_doc.to<JsonArray>();
    
    for (const auto &key : this->keys_) {
      JsonObject json_key = json_keys.createNestedObject();
      json_key["name"] = key.name;
      json_key["mac_address"] = key.mac_address;
      json_key["enabled"] = key.enabled;
      json_key["require_button"] = key.require_button;
      json_key["last_seen"] = key.last_seen;
    }
    
    std::string json_str;
    serializeJson(json_doc, json_str);
    this->registered_keys_sensor_->publish_state(json_str);
  }
}

void BleKeyManager::save_keys_to_flash_() {
  this->flash_storage_.save(&this->keys_);
  ESP_LOGD(TAG, "Chiavi salvate in memoria flash");
}

void BleKeyManager::load_keys_from_flash_() {
  if (!this->flash_storage_.load(&this->keys_)) {
    ESP_LOGW(TAG, "Nessuna chiave trovata in memoria flash");
    return;
  }
  
  ESP_LOGI(TAG, "Caricate %u chiavi dalla memoria flash", this->keys_.size());
}

std::string BleKeyManager::mac_to_string_(const uint64_t &mac) {
  char buffer[18];
  snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
           (uint8_t)(mac >> 40), (uint8_t)(mac >> 32), (uint8_t)(mac >> 24),
           (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)(mac));
  return std::string(buffer);
}

uint64_t BleKeyManager::string_to_mac_(const std::string &mac) {
  uint64_t result = 0;
  uint8_t bytes[6];
  
  sscanf(mac.c_str(), "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
         &bytes[0], &bytes[1], &bytes[2], &bytes[3], &bytes[4], &bytes[5]);
  
  for (int i = 0; i < 6; i++) {
    result = (result << 8) | bytes[i];
  }
  
  return result;
}

void BleKeyManager::start_scan_() {
  ESP_LOGD(TAG, "Avvio scansione BLE");
  this->scanning_ = true;
  this->scan_end_time_ = millis() + this->scan_duration_;
  this->last_scan_time_ = millis();
  
  // Notifica ESPHome per avviare la scansione
  auto *ble = esp32_ble_tracker::global_esp32_ble_tracker;
  if (ble != nullptr) {
    ble->start_scan();
  }
}

void BleKeyManager::stop_scan_() {
  ESP_LOGD(TAG, "Fine scansione BLE");
  this->scanning_ = false;
  
  // Notifica ESPHome per fermare la scansione
  auto *ble = esp32_ble_tracker::global_esp32_ble_tracker;
  if (ble != nullptr) {
    ble->stop_scan();
  }
}

void BleKeyManager::clear_all_keys() {
  ESP_LOGI(TAG, "Cancellazione di tutte le chiavi");
  this->keys_.clear();
  this->save_keys_to_flash_();
  this->update_sensors_();
}

void BleKeyManager::dump_config() {
  ESP_LOGCONFIG(TAG, "BLE Key Manager:");
  ESP_LOGCONFIG(TAG, "  Intervallo di scansione: %u ms", this->scan_interval_);
  ESP_LOGCONFIG(TAG, "  Durata scansione: %u ms", this->scan_duration_);
  ESP_LOGCONFIG(TAG, "  Ripristino da flash: %s", YESNO(this->restore_from_flash_));
  ESP_LOGCONFIG(TAG, "  Chiavi registrate: %u", this->keys_.size());
  
  for (const auto &key : this->keys_) {
    ESP_LOGCONFIG(TAG, "  Chiave: %s", key.name.c_str());
    ESP_LOGCONFIG(TAG, "    MAC: %s", key.mac_address.c_str());
    ESP_LOGCONFIG(TAG, "    Abilitata: %s", YESNO(key.enabled));
    ESP_LOGCONFIG(TAG, "    Richiede pulsante: %s", YESNO(key.require_button));
  }
}

}  // namespace ble_key_manager
}  // namespace esphome