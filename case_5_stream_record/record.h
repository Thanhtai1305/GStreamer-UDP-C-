#ifndef RECORD_H
#define RECORD_H

#include <gst/gst.h>
#include <glib.h>

// Port definitions for sending and receiving video and audio
#define VIDEO_PORT 6000
#define AUDIO_PORT 6002

// Functions to start sender and receiver pipelines
int start_sender(const char *remote_ip);
int start_receiver();

#endif // RECORD_H