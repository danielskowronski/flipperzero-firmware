#include "meteo_sp68.h"

#define TAG "WSProtocolMeteo_SP"

/*
 *
Meteo SP68 sensor.

Transmission details:
- sent over 433.92MHz AM270
- PPM or DPPM (Dufferential Pulse-Position Modulation) 
- each transmission takes place every 30-40 seconds and looks like that:
  - 10x repeated packet taking ~39 ms followed by ~62ms break (1st ocurrence may be corrupted)
  - after last break, there's additional ~192ms break totaling in ~256ms break since last data transmission
  - final ~13ms of unrecognized transmission that's likely just garbage
- all data packets are the same for each transmission

Data packet layout - 42 bits:
      0-DDDDDDDD-HHHHHHHH-XX-CC-TTTTTTTTTTTT-YYYYYYY-00
Ex 1: 0-11001011-00011010-00-00-000010010111-0001010-00
Ex 2: 0-11001011-00110000-00-01-000100100011-1110001-00
Ex 3: 0-11001011-00100110-00-10-000100100000-1001000-00

- 0: zero
- D: id, 8 bit, fixed at least per sensor
- H: humidity value, 8 bit, integer 0-100
- X: unknown data, 2 bit, default 00, low battery reports 10, but couldn't fabricate such flag in random packets
- C: channel ID, 2 bit, values: CH 1 - 00, CH 2 - 01, CH 3 - 10
- T: temperature value, 12 bit, integer storing 10x Celsius degrees
- Y: unknown data, 7 bit, contents doesn't matter
- 0: zero-zero

Example values:
- ex 1: channel 1, 26% humidity, 15.1 C
- ex 2: channel 2, 48% humidity, 29.1 C
- ex 3: channel 3, 38% humidity, 28.8 C

 */

static const SubGhzBlockConst ws_protocol_Meteo_SP68_const = {
    .te_short = 267, // length of short pulse, us
    .te_long  = 633, // length of long pulse, us
    .te_delta = 200, // tolerations, us
    .min_count_bit_for_found = 42,
};

struct WSProtocolDecoderMeteo_SP68 {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    WSBlockGeneric generic;
};

struct WSProtocolEncoderMeteo_SP68 {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    WSBlockGeneric generic;
};

typedef enum {
    Meteo_SP68DecoderStepReset = 0,
    Meteo_SP68DecoderStepSaveDuration,
    Meteo_SP68DecoderStepCheckDuration,
} Meteo_SP68DecoderStep;

const SubGhzProtocolDecoder ws_protocol_Meteo_SP68_decoder = {
    .alloc = ws_protocol_decoder_Meteo_SP68_alloc,
    .free = ws_protocol_decoder_Meteo_SP68_free,

    .feed = ws_protocol_decoder_Meteo_SP68_feed,
    .reset = ws_protocol_decoder_Meteo_SP68_reset,

    .get_hash_data = ws_protocol_decoder_Meteo_SP68_get_hash_data,
    .serialize = ws_protocol_decoder_Meteo_SP68_serialize,
    .deserialize = ws_protocol_decoder_Meteo_SP68_deserialize,
    .get_string = ws_protocol_decoder_Meteo_SP68_get_string,
};

const SubGhzProtocolEncoder ws_protocol_Meteo_SP68_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol ws_protocol_Meteo_SP68 = {
    .name = WS_PROTOCOL_METEO_SP68_NAME,
    .type = SubGhzProtocolWeatherStation,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_315 | SubGhzProtocolFlag_868 |
            SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &ws_protocol_Meteo_SP68_decoder,
    .encoder = &ws_protocol_Meteo_SP68_encoder,
};

void* ws_protocol_decoder_Meteo_SP68_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    WSProtocolDecoderMeteo_SP68* instance = malloc(sizeof(WSProtocolDecoderMeteo_SP68));
    instance->base.protocol = &ws_protocol_Meteo_SP68;
    instance->generic.protocol_name = instance->base.protocol->name;
    return instance;
}

void ws_protocol_decoder_Meteo_SP68_free(void* context) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;
    free(instance);
}

void ws_protocol_decoder_Meteo_SP68_reset(void* context) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;
    instance->decoder.parser_step = Meteo_SP68DecoderStepReset;
}

static bool ws_protocol_Meteo_SP68_check(WSProtocolDecoderMeteo_SP68* instance) {
    if(instance->decoder.decode_count_bit != 42) {
      return false;
    }
    if(((instance->decoder.decode_data >> 23) & 0x7F) > 100) {
      return false;
    }
    return true;
}

/**
 * Analysis of received data
 * @param instance Pointer to a WSBlockGeneric* instance
 */
static void ws_protocol_Meteo_SP68_remote_controller(WSBlockGeneric* instance) {
    instance->id = (instance->data >> 31);
    instance->battery_low = ((instance->data >> 21) & 1);
    instance->channel = ((instance->data >> 19) & 0x7) + 1;
    instance->temp = (float)(int)((instance->data >> 7) & 0x07FF) / 10.0f; 
    instance->humidity = (instance->data >> 23) & 0x7F;

    // sign bit on 1st LSB in 12-bit integer but we don't have such type here so keeping this workaround
    if (instance->temp >= 102.4) { 
      instance->temp -= 204.8;
    }
}


void ws_protocol_decoder_Meteo_SP68_feed(void* context, bool level, uint32_t u_duration) {
    int32_t duration = (int32_t)u_duration;
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;

    // HIGH
    if (level) {
        if(duration > ws_protocol_Meteo_SP68_const.te_long + ws_protocol_Meteo_SP68_const.te_delta) {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        }
        else {
        }
    }

    // LOW
    else {
        // LOW for short amount of time - value 1
        if((duration >=
            ws_protocol_Meteo_SP68_const.te_short - ws_protocol_Meteo_SP68_const.te_delta) &&
           (duration <=
            ws_protocol_Meteo_SP68_const.te_short + ws_protocol_Meteo_SP68_const.te_delta)) {  
            subghz_protocol_blocks_add_bit(&instance->decoder, 1);
        }

        // LOW for long amount of time - value 0
        else if(
            (duration >=
             ws_protocol_Meteo_SP68_const.te_long - ws_protocol_Meteo_SP68_const.te_delta * 2) &&
            (duration <=
             ws_protocol_Meteo_SP68_const.te_long + ws_protocol_Meteo_SP68_const.te_delta * 2)) {
            subghz_protocol_blocks_add_bit(&instance->decoder, 0);
        }

        // LOW for nonstandard time - reset
        else {
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 0;
        }
    }

    // FINALIZE
    if((instance->decoder.decode_count_bit == ws_protocol_Meteo_SP68_const.min_count_bit_for_found) && ws_protocol_Meteo_SP68_check(instance)) {
      instance->generic.data = instance->decoder.decode_data;
      instance->generic.data_count_bit = instance->decoder.decode_count_bit;
      ws_protocol_Meteo_SP68_remote_controller(&instance->generic);
      if(instance->base.callback)
          instance->base.callback(&instance->base, instance->base.context);
    }
}

uint8_t ws_protocol_decoder_Meteo_SP68_get_hash_data(void* context) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus ws_protocol_decoder_Meteo_SP68_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;
    return ws_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    ws_protocol_decoder_Meteo_SP68_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;

    return ws_block_generic_deserialize_check_count_bit(
        &instance->generic, flipper_format, ws_protocol_Meteo_SP68_const.min_count_bit_for_found);
}

void ws_protocol_decoder_Meteo_SP68_get_string(void* context, FuriString* output) {
    furi_assert(context);
    WSProtocolDecoderMeteo_SP68* instance = context;
    furi_string_printf(
        output,
        "%s %dbit\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%lX Ch:%d  Bat:%d\r\n"
        "Temp:%3.1f C Hum:%d%%",
        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32), // TODO: ???
        (uint32_t)(instance->generic.data),
        instance->generic.id,
        instance->generic.channel,
        instance->generic.battery_low,
        (double)instance->generic.temp,
        instance->generic.humidity);
}
