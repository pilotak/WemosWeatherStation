#include <BME280I2C.h>
BME280I2C::Settings settings(
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::OSR_X1,
    BME280::Mode_Forced,
    BME280::StandbyTime_1000ms,
    BME280::Filter_8,
    BME280::SpiEnable_False,
    BMP280_ADDR
);

BME280I2C bmp(settings);

bool setupBaro() {
    if (bmp.begin()) {
        sensor_state |= (1 << 0);

        return true;
    }

#if defined(DEBUG)

    if (sensor_state & 0b1) {
        Serial.println("[SENSOR] BMP280 did not respond. Please check wiring.");
    }

#endif

    sensor_state &= ~(1 << 0);

    return false;
}

void readBaro(sensorData *data) {
#if defined(SENSOR_BMP280)
    float none = NAN;
    bmp.read(data->pressure[1], data->temp[0], none);
#elif defined(SENSOR_BME280)
    bmp.read(data->pressure[1], data->temp[0], data->humidity[0]);
#endif

    if (!isnan(data->temp[0]) && !isnan(data->pressure[1])) {
        data->pressure[0] = round2(baro_filter[0].add(
                                       meteoFunctions.relativePressure_c(
                                           data->pressure[1], height_above_sea, data->temp[0])));
        data->pressure[1] = round2(baro_filter[1].add(data->pressure[1]));
        data->temp[0] = round2(baro_temp_filter.add(data->temp[0]));

#if defined(SENSOR_BME280)
        abs_humidity = round2(humidity_filter[1].add(meteoFunctions.absoluteHumidity_c(data->temp[0], data->humidity[0])));
        data->humidity[0] = round2(humidity_filter[0].add(data->humidity[0]));
        data->temp[2] = data->temp[0];
#endif

    } else {
        sensor_state &= ~(1 << 0);
        baro_filter[0].reset();  // rel
        baro_filter[1].reset();  // abs
        baro_temp_filter.reset();

#if defined(SENSOR_BME280)
        humidity_filter[0].reset();  // rel
        humidity_filter[1].reset();  // abs
#endif

#if defined(DEBUG)
        Serial.println("[SENSOR] Baro stopped responding.");
#endif
    }
}
