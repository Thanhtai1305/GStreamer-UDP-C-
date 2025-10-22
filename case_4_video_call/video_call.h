#ifndef VIDEO_CALL_H
#define VIDEO_CALL_H

#include <gst/gst.h>
#include <glib.h>

// Port definitions for configuration A
#define CONFIG_A_SENDER_VIDEO_PORT 6000
#define CONFIG_A_SENDER_AUDIO_PORT 6002
#define CONFIG_A_RECEIVER_VIDEO_PORT 7000
#define CONFIG_A_RECEIVER_AUDIO_PORT 7002

// Port definitions for configuration B
#define CONFIG_B_SENDER_VIDEO_PORT 7000
#define CONFIG_B_SENDER_AUDIO_PORT 7002
#define CONFIG_B_RECEIVER_VIDEO_PORT 6000
#define CONFIG_B_RECEIVER_AUDIO_PORT 6002

// Function to set up and run video call pipelines
int run_video_call_pipelines(const char *remote_ip, const char *role);

#endif // VIDEO_CALL_H