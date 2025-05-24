/*
 * (c) 2025 aks
 *
 */
#ifndef SIVR_LIB_FSVAD_H
#define SIVR_LIB_FSVAD_H

#ifdef WITH_FREESWITCH
 #include <switch.h>
 #define log_debug(fmt, ...) do{switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, fmt "\n", ##__VA_ARGS__);} while (0)
 #define log_error(fmt, ...) do{switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, fmt "\n", ##__VA_ARGS__);} while (0)
 #define log_warn(fmt, ...) do{switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, fmt "\n", ##__VA_ARGS__);} while (0)
#else
#ifdef WITH_WSTK
 #include <wstk.h>
#else
 #include <stdio.h>
 #define log_debug(fmt, ...) do{printf("DEBUG [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)
 #define log_error(fmt, ...) do{printf("ERROR [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)
 #define log_warn(fmt, ...) do{printf("WARNING [%s:%d]: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);} while (0)
#endif // WITH_WSTK
#endif //WITH_FREESWITCH

typedef enum {
    SIVR_VAD_STATE_NONE,
    SIVR_VAD_STATE_START_TALKING,
    SIVR_VAD_STATE_TALKING,
    SIVR_VAD_STATE_STOP_TALKING,
    SIVR_VAD_STATE_ERROR
} sivr_vad_state_t;

typedef struct sivr_vad_s sivr_vad_t;

/*
 * Valid modes are -1 ("disable fvad, using native"), 0 ("quality"), 1 ("low bitrate"), 2 ("aggressive"), and 3 * ("very aggressive").
 */

sivr_vad_t *sivr_vad_init(int sample_rate, int channels);
int sivr_vad_set_mode(sivr_vad_t *vad, int mode);
void sivr_vad_set_param(sivr_vad_t *vad, const char *key, int val);
sivr_vad_state_t sivr_vad_process(sivr_vad_t *vad, int16_t *data, unsigned int samples);
sivr_vad_state_t sivr_vad_get_state(sivr_vad_t *vad);
void sivr_vad_reset(sivr_vad_t *vad);
void sivr_vad_destroy(sivr_vad_t **vad);

#endif
