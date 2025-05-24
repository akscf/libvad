/*
 * (c) 2025 aks
 * based on freeswitch 1.20.12-release (vad)
 * with built-in libfvad
 *
 */
#include <sivr-vad.h>
#include <fvad.h>

typedef struct sivr_vad_s {
        Fvad *fvad;
        // config
        int channels;
        int sample_rate;
        int debug;
        int divisor;
        int thresh;
        int voice_samples_thresh;
        int silence_samples_thresh;
        //state
        int voice_samples;
        int silence_samples;
        sivr_vad_state_t vad_state;
} sivr_vad_t;


static const char *state2str(sivr_vad_state_t state) {
    switch(state) {
        case SIVR_VAD_STATE_NONE:           return "none";
        case SIVR_VAD_STATE_START_TALKING:  return "start_talking";
        case SIVR_VAD_STATE_TALKING:        return "talking";
        case SIVR_VAD_STATE_STOP_TALKING:   return "stop_talking";
        default: return "error";
    }
}

// ------------------------------------------------------------------------------------------------------------------------------
/**
 **
 **/
sivr_vad_t *sivr_vad_init(int sample_rate, int channels) {
	sivr_vad_t *vad = malloc(sizeof(sivr_vad_t));

	if (!vad) return NULL;

	memset(vad, 0, sizeof(*vad));
	vad->sample_rate = sample_rate ? sample_rate : 8000;
	vad->channels = channels;
	vad->silence_samples_thresh = 500 * (vad->sample_rate / 1000);
	vad->voice_samples_thresh = 200 * (vad->sample_rate / 1000);
	vad->thresh = 100;
	vad->divisor = vad->sample_rate / 8000;

	if (vad->divisor <= 0) {
		vad->divisor = 1;
	}

	sivr_vad_reset(vad);

	return vad;
}

/**
 **  0 = success,
 **/
int sivr_vad_set_mode(sivr_vad_t *vad, int mode) {
    int ret = 0;

	if(mode < 0) {
		if(vad->fvad) {
            fvad_free(vad->fvad);
		}

		vad->fvad = NULL;
		return ret;
	} else if (mode > 3) {
		mode = 3;
	}

	if(!vad->fvad) {
		vad->fvad = fvad_new();

		if(!vad->fvad) {
            log_error("fvad_new()");
            return -1;
		}
	}

	if(vad->fvad) {
		ret = fvad_set_mode(vad->fvad, mode);
		fvad_set_sample_rate(vad->fvad, vad->sample_rate);
	}

#ifdef SIVR_VAD_DEBUG
	log_debug("libfvad started (mode=%d)", mode);
#endif

	return ret;
}

/**
 **
 **/
void sivr_vad_set_param(sivr_vad_t *vad, const char *key, int val) {
	if (!key) return;

	if (!strcmp(key, "silence_ms")) {
		if (val > 0) {
			vad->silence_samples_thresh = val * (vad->sample_rate / 1000);
		} else {
			log_warn("Ignoring invalid silence_ms of %d", val);
		}
	} else if (!strcmp(key, "thresh")) {
		vad->thresh = val;
	} else if (!strcmp(key, "debug")) {
		vad->debug = val;
	} else if (!strcmp(key, "voice_ms")) {
		if (val > 0) {
			vad->voice_samples_thresh = val * (vad->sample_rate / 1000);
		} else {
			log_warn("Ignoring invalid voice_ms of %d", val);
		}
	} else if (!strcmp(key, "listen_hits")) {
		/* convert old-style hits to samples assuming 20ms ptime */
		log_warn("listen_hits is deprecated, setting voice_ms to %d", 20 * val);
		sivr_vad_set_param(vad, "voice_ms", 20 * val);
	}

	if(vad->debug) {
		log_debug("set %s to %d", key, val);
	}
}

void sivr_vad_reset(sivr_vad_t *vad) {
	if (vad->fvad) {
		fvad_reset(vad->fvad);
	}

	vad->vad_state = SIVR_VAD_STATE_NONE;
	vad->voice_samples = 0;
	vad->silence_samples = 0;

	if(vad->debug) {
        log_debug("reset vad state");
	}
}

/**
 **
 **/
sivr_vad_state_t sivr_vad_process(sivr_vad_t *vad, int16_t *data, unsigned int samples) {
	int score = 0;

	// Each frame has 2 possible outcomes- voice or not voice.
	// The VAD has 2 real states- talking / not talking with
	// begin talking and stop talking as events to mark transitions


	// determine if this is a voice or non-voice frame
	if (vad->fvad) {
		// fvad returns -1, 0, or 1
		// -1: error
		//  0: non-voice frame
		//  1: voice frame
		int ret = fvad_process(vad->fvad, data, samples);

		// if voice frame set score > threshold
		score = ret > 0 ? vad->thresh + 100 : 0;
	} else {
		int energy = 0, j = 0, count = 0;
		for (energy = 0, j = 0, count = 0; count < samples; count++) {
			energy += abs(data[j]);
			j += vad->channels;
		}

		if (samples && vad->divisor && samples >= vad->divisor) {
			score = (uint32_t)(energy / (samples / vad->divisor));
		}
	}

	if (vad->debug > 9) {
		log_debug("score: %d", score);
	}

	// clear the STOP/START TALKING events
	if (vad->vad_state == SIVR_VAD_STATE_STOP_TALKING) {
		vad->vad_state = SIVR_VAD_STATE_NONE;
	} else if (vad->vad_state == SIVR_VAD_STATE_START_TALKING) {
		vad->vad_state = SIVR_VAD_STATE_TALKING;
	}

	// adjust voice/silence run length counters
	if (score > vad->thresh) {
		vad->silence_samples = 0;
		vad->voice_samples += samples;
	} else {
		vad->silence_samples += samples;
		vad->voice_samples = 0;
	}

	// check for state transitions
	if (vad->vad_state == SIVR_VAD_STATE_TALKING && vad->silence_samples > vad->silence_samples_thresh) {
		vad->vad_state = SIVR_VAD_STATE_STOP_TALKING;
		if(vad->debug) log_debug("vad state STOP_TALKING");
	} else if (vad->vad_state == SIVR_VAD_STATE_NONE && vad->voice_samples > vad->voice_samples_thresh) {
		vad->vad_state = SIVR_VAD_STATE_START_TALKING;
		if(vad->debug) log_debug("vad state START_TALKING");
	}

	if (vad->debug > 9) {
        log_debug("vad state %s", state2str(vad->vad_state));
	}

	return vad->vad_state;
}

/**
 **
 **/
sivr_vad_state_t sivr_vad_get_state(sivr_vad_t *vad) {
	return vad->vad_state;
}

/**
 **
 **/
void sivr_vad_destroy(sivr_vad_t **vad) {
	if(vad && *vad) {
		if((*vad)->fvad) fvad_free((*vad)->fvad);
		free(*vad);
		*vad = NULL;
	}
}
