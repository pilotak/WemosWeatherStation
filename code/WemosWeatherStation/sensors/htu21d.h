#include <HTU21D.h>
HTU21D htu(HTU21D_RES_RH12_TEMP14);

bool setupHumidity() {
    if (htu.begin(SDA, SCL)) {
        sensor_state |= (1 << 2);

        return true;
    }

#if defined(DEBUG)

    if (sensor_state & 0b100) {
        Serial.println("[SENSOR] HTU21D did not respond. Please check wiring.");
    }

#endif

    sensor_state &= ~(1 << 2);

    return false;
}

void readHumidity(sensorData *data) {
    bool ok = true;

    data->temp[2] = htu.readTemperature();
    data->humidity[0] = htu.readHumidity();

    if (data->temp[2] >= 255.0) {
        data->temp[2] = NAN;
        ok = false;
    }

    if (data->humidity[0] > 100.0 || data->humidity[0] < 0.0) {
        data->humidity[0] = NAN;
        ok = false;
    }

    if (ok) {
        data->humidity[1] = round2(humidity_filter[1].add(meteoFunctions.absoluteHumidity_c(data->temp[2], data->humidity[0])));
        data->humidity[0] = round2(humidity_filter[0].add(data->humidity[0]));
        data->temp[2] = round2(humidity_temp_filter.add(data->temp[2]));

    } else {
        sensor_state &= ~(1 << 2);
        humidity_filter[0].reset();  // rel
        humidity_filter[1].reset();  // abs
        humidity_temp_filter.reset();

#if defined(DEBUG)
        Serial.println("[SENSOR] HTU21D stopped responding.");
#endif
    }
}
