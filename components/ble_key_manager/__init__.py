"""Componente BLE Key Manager per ESPHome."""

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32_ble_tracker
from esphome.const import CONF_ID, CONF_INTERVAL, CONF_DURATION
# Importazioni corrette per le azioni e i trigger
from esphome.automation import Action, register_action, Trigger

DEPENDENCIES = ['esp32_ble_tracker']
AUTO_LOAD = ['sensor', 'text_sensor']

CONF_SCAN_INTERVAL = 'scan_interval'
CONF_SCAN_DURATION = 'scan_duration'
CONF_RESTORE_FROM_FLASH = 'restore_from_flash'

# Definisci il namespace per il componente
ble_key_manager_ns = cg.esphome_ns.namespace('ble_key_manager')
BleKeyManager = ble_key_manager_ns.class_('BleKeyManager', cg.Component, esp32_ble_tracker.ESPBTDeviceListener)

# Azioni per il componente - con l'importazione corretta di Action
AddKeyAction = ble_key_manager_ns.class_('AddKeyAction', Action)
RemoveKeyAction = ble_key_manager_ns.class_('RemoveKeyAction', Action)
SetKeyStatusAction = ble_key_manager_ns.class_('SetKeyStatusAction', Action)
StartScanModeAction = ble_key_manager_ns.class_('StartScanModeAction', Action)

# Schemi di validazione per le azioni
ADD_KEY_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BleKeyManager),
    cv.Required('name'): cv.templatable(cv.string),
    cv.Required('mac_address'): cv.templatable(cv.string),
    cv.Optional('require_button', default=False): cv.templatable(cv.boolean),
})

REMOVE_KEY_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BleKeyManager),
    cv.Required('mac_address'): cv.templatable(cv.string),
})

SET_KEY_STATUS_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BleKeyManager),
    cv.Required('mac_address'): cv.templatable(cv.string),
    cv.Required('enabled'): cv.templatable(cv.boolean),
})

START_SCAN_MODE_ACTION_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BleKeyManager),
    cv.Optional('duration', default='30s'): cv.templatable(cv.positive_time_period_milliseconds),
})

# Configurazione del componente
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(BleKeyManager),
    cv.Optional(CONF_SCAN_INTERVAL, default='1min'): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_SCAN_DURATION, default='5s'): cv.positive_time_period_milliseconds,
    cv.Optional(CONF_RESTORE_FROM_FLASH, default=True): cv.boolean,
}).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

# Registrazione delle azioni - utilizzo della funzione register_action corretta
@register_action('ble_key_manager.add_key', AddKeyAction, ADD_KEY_ACTION_SCHEMA)
async def add_key_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    manager = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_manager(manager))
    
    template_ = await cg.templatable(config['name'], args, cg.std_string)
    cg.add(var.set_name(template_))
    
    template_ = await cg.templatable(config['mac_address'], args, cg.std_string)
    cg.add(var.set_mac_address(template_))
    
    template_ = await cg.templatable(config['require_button'], args, bool)
    cg.add(var.set_require_button(template_))
    
    return var

@register_action('ble_key_manager.remove_key', RemoveKeyAction, REMOVE_KEY_ACTION_SCHEMA)
async def remove_key_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    manager = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_manager(manager))
    
    template_ = await cg.templatable(config['mac_address'], args, cg.std_string)
    cg.add(var.set_mac_address(template_))
    
    return var

@register_action('ble_key_manager.set_key_status', SetKeyStatusAction, SET_KEY_STATUS_ACTION_SCHEMA)
async def set_key_status_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    manager = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_manager(manager))
    
    template_ = await cg.templatable(config['mac_address'], args, cg.std_string)
    cg.add(var.set_mac_address(template_))
    
    template_ = await cg.templatable(config['enabled'], args, bool)
    cg.add(var.set_enabled(template_))
    
    return var

@register_action('ble_key_manager.start_scan_mode', StartScanModeAction, START_SCAN_MODE_ACTION_SCHEMA)
async def start_scan_mode_action_to_code(config, action_id, template_arg, args):
    var = cg.new_Pvariable(action_id, template_arg)
    manager = await cg.get_variable(config[CONF_ID])
    cg.add(var.set_manager(manager))
    
    template_ = await cg.templatable(config['duration'], args, cg.uint32)
    cg.add(var.set_duration(template_))
    
    return var

# Registrazione del trigger per chiave autorizzata rilevata
BLEKeyDetectedTrigger = ble_key_manager_ns.class_('BLEKeyDetectedTrigger', Trigger.template())

ON_KEY_DETECTED_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.use_id(BleKeyManager),
})

@register_action('ble_key_manager.on_authorized_key_detected', Action)
async def ble_key_on_key_detected_to_code(config, action_id, template_arg, args):
    manager = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg)
    cg.add(manager.add_on_authorized_key_detected_callback(var))
    return var

# Funzione principale per generare il codice
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)
    
    if CONF_SCAN_INTERVAL in config:
        cg.add(var.set_scan_interval(config[CONF_SCAN_INTERVAL]))
    
    if CONF_SCAN_DURATION in config:
        cg.add(var.set_scan_duration(config[CONF_SCAN_DURATION]))
    
    if CONF_RESTORE_FROM_FLASH in config:
        cg.add(var.set_restore_from_flash(config[CONF_RESTORE_FROM_FLASH]))
