/**
 * Taken from https://raw.githubusercontent.com/beegee-tokyo/WisBlock-Sensor-For-LoRaWAN/3bc528b03c437b77e1189c804201c874e7370dd0/decoders/TTN-Ext-LPP-Decoder.js
 * @reference https://github.com/myDevicesIoT/cayenne-docs/blob/master/docs/LORA.md
 * @reference http://openmobilealliance.org/wp/OMNA/LwM2M/LwM2MRegistry.html#extlabel
 *
 * Adapted for lora-app-server from https://gist.github.com/iPAS/e24970a91463a4a8177f9806d1ef14b8
 *
 * Type                 IPSO    LPP     Hex     Data Size   Data Resolution per bit
 *  Digital Input       3200    0       0       1           1
 *  Digital Output      3201    1       1       1           1
 *  Analog Input        3202    2       2       2           0.01 Signed
 *  Analog Output       3203    3       3       2           0.01 Signed
 *  Illuminance Sensor  3301    101     65      2           1 Lux Unsigned MSB
 *  Presence Sensor     3302    102     66      1           1
 *  Temperature Sensor  3303    103     67      2           0.1 °C Signed MSB
 *  Humidity Sensor     3304    104     68      1           0.5 % Unsigned
 *  Accelerometer       3313    113     71      6           0.001 G Signed MSB per axis
 *  Barometer           3315    115     73      2           0.1 hPa Unsigned MSB
 *  Time                3333    133     85      4           Unix time MSB
 *  Gyrometer           3334    134     86      6           0.01 °/s Signed MSB per axis
 *  GPS Location        3336    136     88      9           Latitude  : 0.0001 ° Signed MSB
 *                                                          Longitude : 0.0001 ° Signed MSB
 *                                                          Altitude  : 0.01 meter Signed MSB
 *
 * Additional types
 *  Generic Sensor      3300    100     64      4           Unsigned integer MSB
 *  Voltage             3316    116     74      2           0.01 V Unsigned MSB
 *  Current             3317    117     75      2           0.001 A Unsigned MSB
 *  Frequency           3318    118     76      4           1 Hz Unsigned MSB
 *  Percentage          3320    120     78      1           1% Unsigned
 *  Altitude            3321    121     79      2           1m Signed MSB
 *  Concentration       3325    125     7D      2           1 PPM unsigned : 1pmm = 1 * 10 ^-6 = 0.000 001
 *  Power               3328    128     80      2           1 W Unsigned MSB
 *  Distance            3330    130     82      4           0.001m Unsigned MSB
 *  Energy              3331    131     83      4           0.001kWh Unsigned MSB
 *  Colour              3335    135     87      3           R: 255 G: 255 B: 255
 *  Direction           3332    132     84      2           1º Unsigned MSB
 *  Switch              3342    142     8E      1           0/1
 *  GPS Location        3337    137     89      11          Latitude  : 0.000001 ° Signed MSB
 *                                                          Longitude : 0.000001 ° Signed MSB
 *                                                          Altitude  : 0.01 meter Signed MSB
 *  VOC index           3338    138     8A      1           VOC index
 *
 *
 *
 * Specific Sensor Number values
 * Only applies to input from https://github.com/EnviroDIY/USGS_NGWOS/tree/main/NGWOS_TTN
 *
 * Channel  Instrument       Parameter                      Unit
 *   1       Stonefly         Measurement Timestamp (UNIX)   dimensionless
 *   2       mDot             RSSI (signal strength)         dB
 *   3       SHT40            Equipment Temperature          °C
 *   4       SHT40            Relative Humidity              %RH
 *   5       Vega Puls 21     Gauge Height (Stage)           m
 *   6       Vega Puls 21     Range                          m
 *   7       Vega Puls 21     Temperature                    °C
 *   8       Vega Puls 21     Reliability                    dB
 *   9       Vega Puls 21     Error Code                     dimensionless
 *   10      EVERBRIGHT ALS   Luminosity                     lux
 *   11      MAX17048         Battery Voltage                V
 *   12      MAX17048         Battery Charge Percent         %
 *   13      MAX17048         Battery (Dis)Charge Rate       %/hr
 *   14      Meter Hydros 21  Specific Conductance           µS/cm
 *   15      Meter Hydros 21  Temperature                    °C
 *   16      Meter Hydros 21  Water Depth                    m
 */


// lppDecode decodes an array of bytes into an array of ojects,
// each one with the channel, the data type and the value.
function lppDecode(bytes) {

    var sensor_types = {
        0: { 'size': 1, 'name': 'digital_in', 'signed': false, 'divisor': 1 },
        1: { 'size': 1, 'name': 'digital_out', 'signed': false, 'divisor': 1 },
        2: { 'size': 2, 'name': 'analog_in', 'signed': true, 'divisor': 100 },
        3: { 'size': 2, 'name': 'analog_out', 'signed': true, 'divisor': 100 },
        100: { 'size': 4, 'name': 'generic', 'signed': false, 'divisor': 1 },
        101: { 'size': 2, 'name': 'illuminance', 'signed': false, 'divisor': 1 },
        102: { 'size': 1, 'name': 'presence', 'signed': false, 'divisor': 1 },
        103: { 'size': 2, 'name': 'temperature', 'signed': true, 'divisor': 10 },
        104: { 'size': 1, 'name': 'humidity', 'signed': false, 'divisor': 2 },
        113: { 'size': 6, 'name': 'accelerometer', 'signed': true, 'divisor': 1000 },
        115: { 'size': 2, 'name': 'barometer', 'signed': false, 'divisor': 10 },
        116: { 'size': 2, 'name': 'voltage', 'signed': false, 'divisor': 100 },
        117: { 'size': 2, 'name': 'current', 'signed': false, 'divisor': 1000 },
        118: { 'size': 4, 'name': 'frequency', 'signed': false, 'divisor': 1 },
        120: { 'size': 1, 'name': 'percentage', 'signed': false, 'divisor': 1 },
        121: { 'size': 2, 'name': 'altitude', 'signed': true, 'divisor': 1 },
        125: { 'size': 2, 'name': 'concentration', 'signed': false, 'divisor': 1 },
        128: { 'size': 2, 'name': 'power', 'signed': false, 'divisor': 1 },
        130: { 'size': 4, 'name': 'distance', 'signed': false, 'divisor': 1000 },
        131: { 'size': 4, 'name': 'energy', 'signed': false, 'divisor': 1000 },
        132: { 'size': 2, 'name': 'direction', 'signed': false, 'divisor': 1 },
        133: { 'size': 4, 'name': 'time', 'signed': false, 'divisor': 1 },
        134: { 'size': 6, 'name': 'gyrometer', 'signed': true, 'divisor': 100 },
        135: { 'size': 3, 'name': 'colour', 'signed': false, 'divisor': 1 },
        136: { 'size': 9, 'name': 'gps', 'signed': true, 'divisor': [10000, 10000, 100] },
        137: { 'size': 11, 'name': 'gps', 'signed': true, 'divisor': [1000000, 1000000, 100] },
        138: { 'size': 2, 'name': 'voc', 'signed': false, 'divisor': 1 },
        142: { 'size': 1, 'name': 'switch', 'signed': false, 'divisor': 1 },
    };

    var channels = {
        "1": { "instrument": "Stonefly", "parameter": "Measurement Timestamp (UNIX)", "unit": "dimensionless" },
        "2": { "instrument": "mDot", "parameter": "RSSI (signal strength)", "unit": "dB" },
        "3": { "instrument": "SHT40", "parameter": "Equipment Temperature", "unit": "°C" },
        "4": { "instrument": "SHT40", "parameter": "Relative Humidity", "unit": "%RH" },
        "5": { "instrument": "Vega Puls 21", "parameter": "Gauge Height (Stage)", "unit": "m" },
        "6": { "instrument": "Vega Puls 21", "parameter": "Range", "unit": "m" },
        "7": { "instrument": "Vega Puls 21", "parameter": "Temperature", "unit": "°C" },
        "8": { "instrument": "Vega Puls 21", "parameter": "Reliability", "unit": "dB" },
        "9": { "instrument": "Vega Puls 21", "parameter": "Error Code", "unit": "dimensionless" },
        "10": { "instrument": "EVERBRIGHT ALS", "parameter": "Luminosity", "unit": "lux" },
        "11": { "instrument": "MAX17048", "parameter": "Battery Voltage", "unit": "V" },
        "12": { "instrument": "MAX17048", "parameter": "Battery Charge Percent", "unit": "%" },
        "13": { "instrument": "MAX17048", "parameter": "Battery (Dis)Charge Rate", "unit": "%/hr" },
        "14": { "instrument": "Meter Hydros 21", "parameter": "Specific Conductance", "unit": "µS/cm" },
        "15": { "instrument": "Meter Hydros 21", "parameter": "Temperature", "unit": "°C" },
        "16": { "instrument": "Meter Hydros 21", "parameter": "Water Depth", "unit": "m" }
    };

    function arrayToDecimal(stream, is_signed, divisor) {

        var value = 0;
        for (var i = 0; i < stream.length; i++) {
            if (stream[i] > 0xFF)
                throw 'Byte value overflow!';
            value = (value << 8) | stream[i];
        }

        if (is_signed) {
            var edge = 1 << (stream.length) * 8;  // 0x1000..
            var max = (edge - 1) >> 1;             // 0x0FFF.. >> 1
            value = (value > max) ? value - edge : value;
        }

        value /= divisor;

        return value;

    }

    var sensors = {};
    var i = 0;
    var timestamp = 0;
    while (i < bytes.length) {

        // Read the channel N=number
        /** @type {number} */
        var s_no = bytes[i++];
        // Read the parameter type
        /** @type {number} */
        var s_type = bytes[i++];
        if (typeof sensor_types[s_type] == 'undefined') {
            throw 'Sensor type error!: ' + s_type;
        }

        // parse the value based on the sensor type

        /** @type {(number|Object)} */
        var s_value = 0;
        /** @type {Object} */
        var type = sensor_types[s_type];
        switch (s_type) {

            case 113:   // Accelerometer
            case 134:   // Gyrometer
                s_value = {
                    'x': arrayToDecimal(bytes.slice(i + 0, i + 2), type.signed, type.divisor),
                    'y': arrayToDecimal(bytes.slice(i + 2, i + 4), type.signed, type.divisor),
                    'z': arrayToDecimal(bytes.slice(i + 4, i + 6), type.signed, type.divisor)
                };
                break;
            case 136:   // GPS Location
                s_value = {
                    'latitude': arrayToDecimal(bytes.slice(i + 0, i + 3), type.signed, type.divisor[0]),
                    'longitude': arrayToDecimal(bytes.slice(i + 3, i + 6), type.signed, type.divisor[1]),
                    'altitude': arrayToDecimal(bytes.slice(i + 6, i + 9), type.signed, type.divisor[2])
                };
                break;
            case 137:   // Precise GPS Location
                s_value = {
                    'latitude': arrayToDecimal(bytes.slice(i + 0, i + 4), type.signed, type.divisor[0]),
                    'longitude': arrayToDecimal(bytes.slice(i + 4, i + 8), type.signed, type.divisor[1]),
                    'altitude': arrayToDecimal(bytes.slice(i + 8, i + 11), type.signed, type.divisor[2])
                };
                break;
            case 135:   // Colour
                s_value = {
                    'r': arrayToDecimal(bytes.slice(i + 0, i + 1), type.signed, type.divisor),
                    'g': arrayToDecimal(bytes.slice(i + 1, i + 2), type.signed, type.divisor),
                    'b': arrayToDecimal(bytes.slice(i + 2, i + 3), type.signed, type.divisor)
                };
                break;

            case 133: // timestamp
                s_value = arrayToDecimal(bytes.slice(i, i + type.size), type.signed, type.divisor);
                timestamp = s_value;
                break;

            default:    // All the rest
                s_value = arrayToDecimal(bytes.slice(i, i + type.size), type.signed, type.divisor);
                break;
        }

        // Assign sensor and parameter and unit based on channel number
        var s_inst = channels[s_no].instrument;
        var s_param = channels[s_no].parameter;
        var s_unit = channels[s_no].unit;

        sensors[type.name + '_' + s_no] = {
            'channel': s_no,
            // 'type': s_type,
            // 'name': type.name,
            'instrument': s_inst,
            'parameter': s_param,
            'unit': s_unit,
            'value': s_value
        };

        i += type.size;
    }

    return { "timestamp": timestamp, "sensors": sensors };

}

// To use with TTN
function decodeUplink(input) {
    // flat output (like original decoder):
    var response = lppDecode(input.bytes);
    return { data: response };
}
