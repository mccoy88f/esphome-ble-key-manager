import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import text_sensor
from esphome.const import CONF_ID, ICON_KEY
from . import BleKeyManager, ble_key_manager_ns

DEPENDENCIES = ['ble_key_manager']

CONF_MANAGER_ID = 'manager_id'
CONF_REGISTERED_KEYS = 'registered_keys'
CONF_LAST_DETECTED_KEY = 'last_detected_key'

# Schema di configurazione per i sensori di testo
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MANAGER_ID): cv.use_id(BleKeyManager),
    cv.Optional(CONF_REGISTERED_KEYS): text_sensor.text_sensor_schema(
        icon=ICON_KEY
    ),
    cv.Optional(CONF_LAST_DETECTED_KEY): text_sensor.text_sensor_schema(
        icon=ICON_KEY
    ),
})

async def to_code(config):
    manager = await cg.get_variable(config[CONF_MANAGER_ID])
    
    if CONF_REGISTERED_KEYS in config:
        sens = await text_sensor.new_text_sensor(config[CONF_REGISTERED_KEYS])
        cg.add(manager.set_registered_keys_text_sensor(sens))
    
    if CONF_LAST_DETECTED_KEY in config:
        sens = await text_sensor.new_text_sensor(config[CONF_LAST_DETECTED_KEY])
        cg.add(manager.set_last_detected_key_text_sensor(sens))
