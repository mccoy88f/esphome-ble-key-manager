import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import (
    CONF_ID,
    UNIT_EMPTY,
    ICON_KEY,
    STATE_CLASS_MEASUREMENT
)
from . import BleKeyManager, ble_key_manager_ns

DEPENDENCIES = ['ble_key_manager']

CONF_MANAGER_ID = 'manager_id'

# Schema di configurazione per i sensori
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(CONF_MANAGER_ID): cv.use_id(BleKeyManager),
    cv.Optional('name'): sensor.sensor_schema(
        unit_of_measurement=UNIT_EMPTY,
        icon=ICON_KEY,
        accuracy_decimals=0,
        state_class=STATE_CLASS_MEASUREMENT
    ),
})

async def to_code(config):
    manager = await cg.get_variable(config[CONF_MANAGER_ID])
    
    if 'name' in config:
        sens = await sensor.new_sensor(config['name'])
        cg.add(manager.set_number_of_keys_sensor(sens))
