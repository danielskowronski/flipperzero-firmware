#pragma once

#include <lib/subghz/protocols/base.h>

#include <lib/subghz/blocks/const.h>
#include <lib/subghz/blocks/decoder.h>
#include <lib/subghz/blocks/encoder.h>
#include "ws_generic.h"
#include <lib/subghz/blocks/math.h>

#define WS_PROTOCOL_METEO_SP68_NAME "Meteo SP68" //Meteo SP68

typedef struct WSProtocolDecoderMeteo_SP68 WSProtocolDecoderMeteo_SP68;
typedef struct WSProtocolEncoderMeteo_SP68 WSProtocolEncoderMeteo_SP68;

extern const SubGhzProtocolDecoder ws_protocol_Meteo_SP68_decoder;
extern const SubGhzProtocolEncoder ws_protocol_Meteo_SP68_encoder;
extern const SubGhzProtocol ws_protocol_Meteo_SP68;

/**
 * Allocate WSProtocolDecoderMeteo_SP68.
 * @param environment Pointer to a SubGhzEnvironment instance
 * @return WSProtocolDecoderMeteo_SP68* pointer to a WSProtocolDecoderMeteo_SP68 instance
 */
void* ws_protocol_decoder_Meteo_SP68_alloc(SubGhzEnvironment* environment);

/**
 * Free WSProtocolDecoderMeteo_SP68.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 */
void ws_protocol_decoder_Meteo_SP68_free(void* context);

/**
 * Reset decoder WSProtocolDecoderMeteo_SP68.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 */
void ws_protocol_decoder_Meteo_SP68_reset(void* context);

/**
 * Parse a raw sequence of levels and durations received from the air.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 * @param level Signal level true-high false-low
 * @param duration Duration of this level in, us
 */
void ws_protocol_decoder_Meteo_SP68_feed(void* context, bool level, uint32_t duration);

/**
 * Getting the hash sum of the last randomly received parcel.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 * @return hash Hash sum
 */
uint8_t ws_protocol_decoder_Meteo_SP68_get_hash_data(void* context);

/**
 * Serialize data WSProtocolDecoderMeteo_SP68.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @param preset The modulation on which the signal was received, SubGhzRadioPreset
 * @return status
 */
SubGhzProtocolStatus ws_protocol_decoder_Meteo_SP68_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

/**
 * Deserialize data WSProtocolDecoderMeteo_SP68.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 * @param flipper_format Pointer to a FlipperFormat instance
 * @return status
 */
SubGhzProtocolStatus
    ws_protocol_decoder_Meteo_SP68_deserialize(void* context, FlipperFormat* flipper_format);

/**
 * Getting a textual representation of the received data.
 * @param context Pointer to a WSProtocolDecoderMeteo_SP68 instance
 * @param output Resulting text
 */
void ws_protocol_decoder_Meteo_SP68_get_string(void* context, FuriString* output);
