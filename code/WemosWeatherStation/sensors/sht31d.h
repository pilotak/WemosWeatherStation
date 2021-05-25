#include <ClosedCube_SHT31D.h>
ClosedCube_SHT31D sht31;

bool setupHumidity() {
    if (sht31.begin(SHT31D_ADDR) == SHT3XD_NO_ERROR) {
        if (sht31.periodicStart(SHT3XD_REPEATABILITY_HIGH, SHT3XD_FREQUENCY_10HZ) == SHT3XD_NO_ERROR) {
            sensor_state |= (1 << 2);

            return true;
        }
    }

#if defined(DEBUG)

    if (sensor_state & 0b100) {
        Serial.println("[SENSOR] SHT31D did not respond. Please check wiring.");
    }

#endif

    sensor_state &= ~(1 << 2);

    return false;
}

void readHumidity(sensorData *data) {
    SHT31D result = sht31.periodicFetchData();

    if (result.error == SHT3XD_NO_ERROR) {
        data->humidity[1] = round2(humidity_filter[1].add(meteoFunctions.absoluteHumidity_c(result.t, result.rh)));
        data->humidity[0] = round2(humidity_filter[0].add(result.rh));
        data->temp[2] = round2(humidity_temp_filter.add(result.t));

    } else {
        sensor_state &= ~(1 << 2);
        humidity_filter[0].reset();  // rel
        humidity_filter[1].reset();  // abs
        humidity_temp_filter.reset();

#if defined(DEBUG)
        Serial.println("[SENSOR] SHT31D stopped responding.");
#endif
    }
}
