/*
 * channel_emulator
 * Copyright 2015 Samsung R&D Institute Ukraine
 * All rights reserved.
 *
 *  @author: Volodymyr Brynza <v dot brynza at samsung dot com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <stdio.h>
#include <string.h>

#include "gstchannelemulator.h"

GST_DEBUG_CATEGORY_STATIC (gst_channel_emulator_debug);
#define GST_CAT_DEFAULT gst_channel_emulator_debug

enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_RAND_SEED,
  PROP_MIN_DELAY,
  PROP_MAX_DELAY,
  PROP_DELAY_PROBABILITY,
  PROP_DROP_MODE,
  PROP_DROP_PROBABILITY,
  PROP_BURST_PROBABILITY,
  PROP_BURST_LENGTH,
  PROP_MAX_DROP,
  PROP_PERIOD,
  PROP_USER_MODEL_FILE,
  PROP_MODEL_PARAM,
  PROP_BIT_FLIP_PROBABILITY,
  PROP_BIT_FLIP_LENGTH,
  PROP_BIT_FLIP_PACKETS,
  PROP_DATA_CORRUPT_PROBABILITY,
  PROP_DATA_CORRUPT_BYTES,
  PROP_DATA_CORRUPT_PACKETS,
  PROP_DUPLICATE_PROBABILITY
};

typedef struct
{
  GstChannelEmulator *emul;
  GstBuffer *buffer;
} SourceInfo;

#define DEFAULT_RAND_SEED G_MAXUINT32
#define DEFAULT_MIN_DELAY 2000
#define DEFAULT_MAX_DELAY 4000
#define DEFAULT_DELAY_PROBABILITY 0.0
#define DEFAULT_DROP_MODE GST_CHANNEL_EMULATOR_RANDOM_DROP
#define DEFAULT_DROP_PROBABILITY 0.0
#define DEFAULT_BURST_PROBABILITY 0.0
#define DEFAULT_MAX_BURST_LENGTH 5
#define DEFAULT_LOOP_RETURN_PERIOD 1000
#define DEFAULT_MAX_DROP_PACKET DEFAULT_LOOP_RETURN_PERIOD
#define DEFAULT_DUPLICATE_PROBABILITY 0.0
#define DEFAULT_BIT_FLIP_PROBABILITY 0.0
#define DEFAULT_BIT_FLIP_LENGTH -1
#define DEFAULT_BIT_FLIP_PACKETS DEFAULT_LOOP_RETURN_PERIOD
#define DEFAULT_CORRUPT_PROBABILITY 0.0
#define DEFAULT_CORRUPT_LEGTH 5
#define DEFAULT_CORRUPT_PACKETS DEFAULT_LOOP_RETURN_PERIOD

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

#define gst_channel_emulator_parent_class parent_class
G_DEFINE_TYPE (GstChannelEmulator, gst_channel_emulator, GST_TYPE_ELEMENT);

static void gst_channel_emulator_finalize (GObject * object);
static void gst_channel_emulator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_channel_emulator_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_channel_emulator_sink_event (GstPad * pad,
    GstObject * parent, GstEvent * event);
static GstFlowReturn gst_channel_emulator_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);

#define GST_TYPE_DROP_MODE (gst_channel_emulator_drop_mode_get_type ())
static GType
gst_channel_emulator_drop_mode_get_type (void)
{
  static GType channel_emulator_drop_mode_type = 0;
  static const GEnumValue drop_mode[] = {
    {GST_CHANNEL_EMULATOR_RANDOM_DROP, "Random drop mode", "random"},
    {GST_CHANNEL_EMULATOR_BURST_DROP, "Burst drop mode", "burst"},
    {GST_CHANNEL_EMULATOR_COMBINED_DROP, "Combined random and burst drop mode",
        "combined"},
    {GST_CHANNEL_EMULATOR_MC_MODEL_DROP,
        "4-state Markov chain model driven drop mode", "mcmodel"},
    {GST_CHANNEL_EMULATOR_GE_MODEL_DROP,
        "Gilbert-Elliott model driven drop mode", "gemodel"},
    {GST_CHANNEL_EMULATOR_USER_MODEL_DROP, "User model for packet droping",
        "userfile"},
    {0, NULL, NULL},
  };

  if (channel_emulator_drop_mode_type == 0) {
    channel_emulator_drop_mode_type =
        g_enum_register_static ("GstChannelEmulatorDropMode", drop_mode);
  }
  return channel_emulator_drop_mode_type;
}

/* GObject vmethod implementations */

/* initialize the plugin's class */
static void
gst_channel_emulator_class_init (GstChannelEmulatorClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_channel_emulator_set_property;
  gobject_class->get_property = gst_channel_emulator_get_property;
  gobject_class->finalize = gst_channel_emulator_finalize;

  g_object_class_install_property (gobject_class, PROP_RAND_SEED,
      g_param_spec_uint64 ("rand-seed",
          "Random seed",
          "Seed for random number generator",
          0, G_MAXUINT32, DEFAULT_RAND_SEED, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MIN_DELAY,
      g_param_spec_int ("min-delay",
          "Minimum delay (ms)",
          "The minimum delay in ms to apply to buffers",
          0, G_MAXINT, DEFAULT_MIN_DELAY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_MAX_DELAY,
      g_param_spec_int ("max-delay",
          "Maximum delay (ms)",
          "The maximum delay in ms to apply to buffers",
          0, G_MAXINT, DEFAULT_MAX_DELAY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DELAY_PROBABILITY, g_param_spec_float ("delay-probability",
          "Delay Probability",
          "The Probability a buffer is delayed", 0.0, 1.0,
          DEFAULT_DELAY_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DROP_MODE, g_param_spec_enum ("drop-mode",
          "Drop mode to use",
          "Drop mode to use",
          GST_TYPE_DROP_MODE, DEFAULT_DROP_MODE,
          (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));

  g_object_class_install_property (gobject_class,
      PROP_DROP_PROBABILITY, g_param_spec_float ("drop-probability",
          "Drop Probability",
          "The Probability a buffer is dropped", 0.0, 1.0,
          DEFAULT_DROP_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_BURST_PROBABILITY, g_param_spec_float ("burst-probability",
          "Burst Probability",
          "The Probability a buffer will be dropped by burst", 0.0, 1.0,
          DEFAULT_BURST_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_BURST_LENGTH, g_param_spec_int ("burst-length",
          "Maximum length of burst packet series",
          "Maximum length of burst packet series", 0, G_MAXINT,
          DEFAULT_MAX_BURST_LENGTH, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_MAX_DROP, g_param_spec_int ("max-drop-count",
          "Maximum Drop Count",
          "Maximum count of packet that was dropped over within work period", 0,
          G_MAXINT, DEFAULT_MAX_DROP_PACKET, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_PERIOD, g_param_spec_int ("loop-return-period",
          "Loop Return Period",
          "The period in which fixed number of packets will be dropped", 0,
          G_MAXINT, DEFAULT_LOOP_RETURN_PERIOD, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DUPLICATE_PROBABILITY, g_param_spec_float ("duplicate-probability",
          "Duplicate Probability",
          "The Probability a buffer is duplicated", 0.0, 1.0,
          DEFAULT_DUPLICATE_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_USER_MODEL_FILE, g_param_spec_string ("user-model",
          "User Model File",
          "Location of file with user specified model of packet drop", NULL,
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_MODEL_PARAM, g_param_spec_boxed ("model-param",
          "Model Parameter",
          "Structure with parameter for packets drop model",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_BIT_FLIP_PROBABILITY, g_param_spec_float ("bitflip-probability",
          "Bit flip Probability",
          "The Probability a that bytes in buffer was inverted", 0.0, 1.0,
          DEFAULT_BIT_FLIP_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_BIT_FLIP_LENGTH, g_param_spec_int ("bit-length",
          "Bit Flip Length",
          "Period in bytes where bits will be inverted", -1, G_MAXINT,
          DEFAULT_BIT_FLIP_LENGTH, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_BIT_FLIP_PACKETS, g_param_spec_int ("bit-flip-packet",
          "Number of packets bit inversed",
          "Number of packets that has inverted bits", 0, G_MAXINT,
          DEFAULT_BIT_FLIP_PACKETS, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DATA_CORRUPT_PROBABILITY,
      g_param_spec_float ("data-corrupt-probability",
          "Data Corruption Probability",
          "The Probability that end of packet was changed to 0", 0.0, 1.0,
          DEFAULT_CORRUPT_PROBABILITY, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DATA_CORRUPT_BYTES, g_param_spec_int ("corrupt-length",
          "Data Corruption Length",
          "Number of bytes which would be set to 0 from the end of packet", 0,
          G_MAXINT, DEFAULT_CORRUPT_LEGTH, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
      PROP_DATA_CORRUPT_PACKETS, g_param_spec_int ("corrupt-packet-number",
          "Number of corrupted packets",
          "Number of packets that was corrupted", 0, G_MAXINT,
          DEFAULT_CORRUPT_PACKETS, G_PARAM_READWRITE));

  gst_element_class_set_details_simple (gstelement_class,
      "ChannelEmulator",
      "Network/Utility", "Network channel emulator", "Samsung Electronics");

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&src_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_channel_emulator_init (GstChannelEmulator * emul)
{
  emul->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
  gst_pad_set_event_function (emul->sinkpad,
      GST_DEBUG_FUNCPTR (gst_channel_emulator_sink_event));
  gst_pad_set_chain_function (emul->sinkpad,
      GST_DEBUG_FUNCPTR (gst_channel_emulator_chain));
  GST_PAD_SET_PROXY_CAPS (emul->sinkpad);
  gst_element_add_pad (GST_ELEMENT (emul), emul->sinkpad);

  emul->srcpad = gst_pad_new_from_static_template (&src_factory, "src");
  GST_PAD_SET_PROXY_CAPS (emul->srcpad);
  gst_element_add_pad (GST_ELEMENT (emul), emul->srcpad);

  emul->min_delay = DEFAULT_MIN_DELAY;
  emul->max_delay = DEFAULT_MAX_DELAY;
  emul->delay_probability = DEFAULT_DELAY_PROBABILITY;
  emul->drop_mode = DEFAULT_DROP_MODE;
  emul->drop_probability = DEFAULT_DROP_PROBABILITY;
  emul->burst_probability = DEFAULT_BURST_PROBABILITY;
  emul->burst_start = FALSE;
  emul->current_burst = 0;
  emul->burst_length = DEFAULT_MAX_BURST_LENGTH;
  emul->max_drop_packet = DEFAULT_MAX_DROP_PACKET;
  emul->dropped_packet = 0;
  emul->processed_packet = 0;
  emul->period = DEFAULT_LOOP_RETURN_PERIOD;
  emul->duplicate_probability = DEFAULT_DUPLICATE_PROBABILITY;
  emul->rand = g_rand_new ();
  emul->bit_mask = (guint8) g_rand_int_range (emul->rand, 0, 255);
  emul->bit_lost_probability = DEFAULT_BIT_FLIP_PROBABILITY;
  emul->bit_lost_byte_length = DEFAULT_BIT_FLIP_LENGTH;
  emul->bit_lost_number = DEFAULT_BIT_FLIP_PACKETS;
  emul->current_bit_lost = 0;
  emul->packet_corrupt_probability = DEFAULT_CORRUPT_PROBABILITY;
  emul->packet_corrupt_bytes = DEFAULT_CORRUPT_LEGTH;
  emul->packet_corrupt_number = DEFAULT_CORRUPT_PACKETS;
  emul->current_packet_corrupt = 0;
  emul->user_drop_file = NULL;
  emul->user_drop_model = NULL;
  emul->processed_by_model = 0;
}

static void
gst_channel_emulator_finalize (GObject * object)
{
  GstChannelEmulator *emul = NULL;
  emul = GST_CHANNEL_EMULATOR (object);

  g_free (emul->user_drop_file);
  g_free (emul->user_drop_model);

  if (emul->model_param)
    gst_structure_free (emul->model_param);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gst_channel_emulator_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstChannelEmulator *emul = GST_CHANNEL_EMULATOR (object);

  switch (prop_id) {
    case PROP_RAND_SEED:
      emul->rand_seed = g_value_get_uint64 (value);
      if (emul->rand_seed != G_MAXUINT32) {
        g_rand_set_seed (emul->rand, emul->rand_seed);
        emul->bit_mask = (guint8) g_rand_int_range (emul->rand, 0, 255);
      }
      break;
    case PROP_MIN_DELAY:
      emul->min_delay = g_value_get_int (value);
      break;
    case PROP_MAX_DELAY:
      emul->max_delay = g_value_get_int (value);
      break;
    case PROP_DELAY_PROBABILITY:
      emul->delay_probability = g_value_get_float (value);
      break;
    case PROP_DROP_MODE:
      emul->drop_mode = g_value_get_enum (value);
      if (emul->drop_mode == GST_CHANNEL_EMULATOR_MC_MODEL_DROP ||
          emul->drop_mode == GST_CHANNEL_EMULATOR_GE_MODEL_DROP) {
        emul->s.state = GST_CHANNEL_EMULATOR_TRANSMITED;
        emul->s.p1 = g_rand_double_range (emul->rand, 0, 1);
        emul->s.p2 = 1. - emul->s.p1;
        emul->s.p3 = 0.0;
        emul->s.p4 = 0.0;
        emul->s.p5 = 1.0;
      }
      break;
    case PROP_DROP_PROBABILITY:
      emul->drop_probability = g_value_get_float (value);
      break;
    case PROP_BURST_PROBABILITY:
      emul->burst_probability = g_value_get_float (value);
      break;
    case PROP_BURST_LENGTH:
      emul->burst_length = g_value_get_int (value);
      break;
    case PROP_MAX_DROP:
      emul->max_drop_packet = g_value_get_int (value);
      break;
    case PROP_PERIOD:
      emul->period = g_value_get_int (value);
      break;
    case PROP_DUPLICATE_PROBABILITY:
      emul->duplicate_probability = g_value_get_float (value);
      break;
    case PROP_USER_MODEL_FILE:
      g_free (emul->user_drop_file);
      g_free (emul->user_drop_model);
      emul->user_drop_file = g_strdup (g_value_get_string (value));
      if (emul->user_drop_file != NULL &&
          g_file_get_contents (emul->user_drop_file,
              &emul->user_drop_model, &emul->model_len, NULL)) {
        GST_INFO_OBJECT (emul, "Read user model from file success");
      } else {
        GST_WARNING_OBJECT (emul, "Can't read user model from file");
      }
      break;
    case PROP_MODEL_PARAM:
    {
      const GstStructure *s = gst_value_get_structure (value);
      if (emul->model_param)
        gst_structure_free (emul->model_param);
      if (s) {
        emul->model_param = gst_structure_copy (s);
        if (gst_structure_has_field (s, "p1"))
          gst_structure_get_double (s, "p1", &emul->s.p1);
        if (gst_structure_has_field (s, "p2"))
          gst_structure_get_double (s, "p2", &emul->s.p2);
        if (gst_structure_has_field (s, "p3"))
          gst_structure_get_double (s, "p3", &emul->s.p3);
        if (gst_structure_has_field (s, "p4"))
          gst_structure_get_double (s, "p4", &emul->s.p4);
        if (gst_structure_has_field (s, "p5"))
          gst_structure_get_double (s, "p5", &emul->s.p5);
      } else
        emul->model_param = NULL;
      break;
    }
    case PROP_BIT_FLIP_PROBABILITY:
      emul->bit_lost_probability = g_value_get_float (value);
      break;
    case PROP_BIT_FLIP_LENGTH:
      emul->bit_lost_byte_length = g_value_get_int (value);
      break;
    case PROP_BIT_FLIP_PACKETS:
      emul->bit_lost_number = g_value_get_int (value);
      break;
    case PROP_DATA_CORRUPT_PROBABILITY:
      emul->packet_corrupt_probability = g_value_get_float (value);
      break;
    case PROP_DATA_CORRUPT_BYTES:
      emul->packet_corrupt_bytes = g_value_get_int (value);
      break;
    case PROP_DATA_CORRUPT_PACKETS:
      emul->packet_corrupt_number = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_channel_emulator_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstChannelEmulator *emul = GST_CHANNEL_EMULATOR (object);

  switch (prop_id) {
    case PROP_RAND_SEED:
      g_value_set_uint64 (value, emul->rand_seed);
      break;
    case PROP_MIN_DELAY:
      g_value_set_int (value, emul->min_delay);
      break;
    case PROP_MAX_DELAY:
      g_value_set_int (value, emul->max_delay);
      break;
    case PROP_DELAY_PROBABILITY:
      g_value_set_float (value, emul->delay_probability);
      break;
    case PROP_DROP_MODE:
      g_value_set_enum (value, emul->drop_mode);
      break;
    case PROP_DROP_PROBABILITY:
      g_value_set_float (value, emul->drop_probability);
      break;
    case PROP_BURST_PROBABILITY:
      g_value_set_float (value, emul->burst_probability);
      break;
    case PROP_BURST_LENGTH:
      g_value_set_int (value, emul->burst_length);
      break;
    case PROP_MAX_DROP:
      g_value_set_int (value, emul->max_drop_packet);
      break;
    case PROP_PERIOD:
      g_value_set_int (value, emul->period);
      break;
    case PROP_DUPLICATE_PROBABILITY:
      g_value_set_float (value, emul->duplicate_probability);
      break;
    case PROP_USER_MODEL_FILE:
      g_value_set_string (value, emul->user_drop_file);
      break;
    case PROP_MODEL_PARAM:
      gst_value_set_structure (value, emul->model_param);
      break;
    case PROP_BIT_FLIP_PROBABILITY:
      g_value_set_float (value, emul->bit_lost_probability);
      break;
    case PROP_BIT_FLIP_LENGTH:
      g_value_set_int (value, emul->bit_lost_byte_length);
      break;
    case PROP_BIT_FLIP_PACKETS:
      g_value_set_int (value, emul->bit_lost_number);
      break;
    case PROP_DATA_CORRUPT_PROBABILITY:
      g_value_set_float (value, emul->packet_corrupt_probability);
      break;
    case PROP_DATA_CORRUPT_BYTES:
      g_value_set_int (value, emul->packet_corrupt_bytes);
      break;
    case PROP_DATA_CORRUPT_PACKETS:
      g_value_set_int (value, emul->packet_corrupt_number);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement virtual method implementations */

/* this function handles sink events */
static gboolean
gst_channel_emulator_sink_event (GstPad * pad, GstObject * parent,
    GstEvent * event)
{
  gboolean ret;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;

      gst_event_parse_caps (event, &caps);

      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  return ret;
}

static gboolean
push_buffer (SourceInfo * info)
{
  GST_DEBUG_OBJECT (info->emul, "Pushing buffer now");
  gst_pad_push (info->emul->srcpad, info->buffer);
  g_free (info);

  return FALSE;
}

static GstFlowReturn
gst_channel_emulator_delay_buffer (GstChannelEmulator * emul, GstPad * pad,
    GstBuffer * buf)
{
  if (emul->delay_probability > 0) {
    if (g_rand_double_range (emul->rand, 0, 1) < emul->delay_probability) {
      SourceInfo *info = g_new0 (SourceInfo, 1);
      gint delay;
      info->emul = emul;
      info->buffer = buf;
      delay = g_rand_int_range (emul->rand, emul->min_delay, emul->max_delay);
      GST_DEBUG_OBJECT (emul, "Delaying packet by %d", delay);
      g_timeout_add (delay, (GSourceFunc) push_buffer, info);

      return GST_FLOW_OK;
    }
  }

  return gst_pad_push (pad, buf);
}

static GstFlowReturn
gst_channel_emulator_data_corrupt (GstChannelEmulator * emul, GstPad * pad,
    GstBuffer * buf)
{
  GstMapInfo buf_info = GST_MAP_INFO_INIT;
  guint i = 0;

  if (!gst_buffer_is_writable (buf)) {
    GstBuffer *old;
    old = buf;
    buf = gst_buffer_copy (old);
    gst_buffer_unref (old);
  }

  gst_buffer_map (buf, &buf_info, GST_MAP_WRITE);

  if (emul->bit_lost_probability > 0) {
    if (g_rand_double_range (emul->rand, 0, 1) < emul->bit_lost_probability
        && emul->current_bit_lost < emul->bit_lost_number) {
      guint length = 0;
      if (emul->bit_lost_byte_length == -1) {
        length = buf_info.size;
      } else {
        length = emul->bit_lost_byte_length;
      }

      for (i = 0; i < length && i < buf_info.size; i++) {
        buf_info.data[i] ^= emul->bit_mask;
      }
      GST_DEBUG_OBJECT (emul, "Bit corruption added to packet");
      emul->current_bit_lost++;
    }
  }

  if (emul->packet_corrupt_probability > 0) {
    if (g_rand_double_range (emul->rand, 0, 1) <
        emul->packet_corrupt_probability
        && emul->current_packet_corrupt < emul->packet_corrupt_number) {
      guint size = 0;
      if (emul->packet_corrupt_bytes > buf_info.size) {
        i = 0;
        size = buf_info.size;
      } else {
        i = buf_info.size - emul->packet_corrupt_bytes;
        size = emul->packet_corrupt_bytes;
      }
      memset (buf_info.data + i, 0, size);
      GST_DEBUG_OBJECT (emul, "%d bytes erased from input packet", size);
      emul->current_packet_corrupt++;
    }
  }

  gst_buffer_unmap (buf, &buf_info);
  return gst_channel_emulator_delay_buffer (emul, pad, buf);
}

static gboolean
gst_channel_emulator_markov_chain_drop (GstChannelEmulator * emul)
{
  gfloat r = g_rand_double_range (emul->rand, 0, 1);

  switch (emul->s.state) {
    case GST_CHANNEL_EMULATOR_TRANSMITED:
      if (r < emul->s.p4) {
        emul->s.state = GST_CHANNEL_EMULATOR_SINGLE_LOST;
        return TRUE;
      } else if (emul->s.p4 < r && r < emul->s.p1 + emul->s.p4) {
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_LOST;
        return TRUE;
      } else if (emul->s.p1 + emul->s.p4 < r) {
        emul->s.state = GST_CHANNEL_EMULATOR_TRANSMITED;
      }
      break;
    case GST_CHANNEL_EMULATOR_BURST_LOST:
      if (r < emul->s.p3) {
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_TRANSMITED;
      } else if (emul->s.p3 < r && r < emul->s.p2 + emul->s.p3) {
        emul->s.state = GST_CHANNEL_EMULATOR_TRANSMITED;
        return TRUE;
      } else if (emul->s.p2 + emul->s.p3 < r) {
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_LOST;
        return TRUE;
      }
    case GST_CHANNEL_EMULATOR_BURST_TRANSMITED:
      if (r < emul->s.p5) {
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_DROP;
        return TRUE;
      } else {
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_TRANSMITED;
      }
    case GST_CHANNEL_EMULATOR_SINGLE_LOST:
      emul->s.state = GST_CHANNEL_EMULATOR_TRANSMITED;
      break;
  }

  return FALSE;
}

static gboolean
gst_channel_emulator_gilb_ell_drop (GstChannelEmulator * emul)
{
  switch (emul->s.state) {
    case GST_CHANNEL_EMULATOR_TRANSMITED:
      if (g_rand_double_range (emul->rand, 0, 1) < emul->s.p1)
        emul->s.state = GST_CHANNEL_EMULATOR_BURST_DROP;
      if (g_rand_double_range (emul->rand, 0, 1) < emul->s.p4)
        return TRUE;
      break;
    case GST_CHANNEL_EMULATOR_BURST_DROP:
      if (g_rand_double_range (emul->rand, 0, 1) < emul->s.p2)
        emul->s.state = GST_CHANNEL_EMULATOR_TRANSMITED;
      if (emul->s.p3 > g_rand_double_range (emul->rand, 0, 1))
        return TRUE;
      break;
    default:
      break;
  }

  return FALSE;
}

static gboolean
gst_channel_emulator_model_drop (GstChannelEmulator * emul)
{
  if (emul->drop_mode == GST_CHANNEL_EMULATOR_MC_MODEL_DROP &&
      gst_channel_emulator_markov_chain_drop (emul)) {
    GST_DEBUG_OBJECT (emul, "Markov model drop packet");
    return TRUE;
  }

  if (emul->drop_mode == GST_CHANNEL_EMULATOR_GE_MODEL_DROP &&
      gst_channel_emulator_gilb_ell_drop (emul)) {
    GST_DEBUG_OBJECT (emul, "Gilbert-Elliott model drop packet");
    return TRUE;
  }

  return FALSE;
}

static gboolean
gst_channel_emulator_user_model_drop (GstChannelEmulator * emul)
{
  if (emul->drop_mode == GST_CHANNEL_EMULATOR_USER_MODEL_DROP
      && emul->user_drop_model != NULL) {
    if (emul->processed_by_model == emul->model_len + 1) {
      emul->processed_by_model = 0;
    }
    emul->processed_by_model++;
    if (emul->user_drop_model[emul->processed_by_model - 1] == '1') {
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
gst_channel_emulator_random_drop (GstChannelEmulator * emul)
{
  if (emul->burst_probability > 0 &&
      (emul->drop_mode == GST_CHANNEL_EMULATOR_BURST_DROP ||
          emul->drop_mode == GST_CHANNEL_EMULATOR_COMBINED_DROP) &&
      !emul->burst_start) {
    if ((gfloat) g_rand_double_range (emul->rand, 0, 1) <
        emul->burst_probability && emul->dropped_packet < emul->max_drop_packet)
    {
      emul->burst_start = TRUE;
    }
  }

  if (emul->burst_start && emul->dropped_packet < emul->max_drop_packet) {
    if (emul->current_burst < emul->burst_length) {
      emul->dropped_packet++;
      emul->current_burst++;
      GST_DEBUG_OBJECT (emul, "Burst occurred");
      return TRUE;
    } else {
      emul->burst_start = FALSE;
      emul->current_burst = 0;
    }
  } else {
    if (emul->drop_probability > 0 &&
        (emul->drop_mode == GST_CHANNEL_EMULATOR_RANDOM_DROP ||
            emul->drop_mode == GST_CHANNEL_EMULATOR_COMBINED_DROP)) {
      if ((gfloat) g_rand_double_range (emul->rand, 0, 1) <
          emul->drop_probability
          && emul->dropped_packet < emul->max_drop_packet) {
        emul->dropped_packet++;
        return TRUE;
      }
    }
  }

  return FALSE;
}

static gboolean
gst_channel_emulator_drop_packet (GstChannelEmulator * emul)
{
  if (gst_channel_emulator_model_drop (emul)) {
    return TRUE;
  }

  if (gst_channel_emulator_user_model_drop (emul)) {
    return TRUE;
  }

  if (gst_channel_emulator_random_drop (emul)) {
    return TRUE;
  }

  return FALSE;
}

static void
gst_channel_emulator_duplicate (GstChannelEmulator * emul, GstBuffer * buf)
{
  if (emul->duplicate_probability > 0) {
    if ((gfloat) g_rand_double_range (emul->rand, 0, 1) <
        emul->duplicate_probability) {
      GST_DEBUG_OBJECT (emul, "Duplicating packet");
      gst_buffer_ref (buf);
      gst_channel_emulator_data_corrupt (emul, emul->srcpad, buf);
    }
  }
}

static GstFlowReturn
gst_channel_emulator_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstChannelEmulator *emul;

  emul = GST_CHANNEL_EMULATOR (parent);

  emul->processed_packet++;
  if (emul->processed_packet >= emul->period) {
    emul->dropped_packet = 0;
    emul->processed_packet = 0;
    emul->current_bit_lost = 0;
    emul->current_packet_corrupt = 0;
  }

  if (gst_channel_emulator_drop_packet (emul)) {
    GST_DEBUG_OBJECT (emul, "Dropping packet");
    gst_buffer_unref (buf);
    return GST_FLOW_OK;
  }

  gst_channel_emulator_duplicate (emul, buf);

  return gst_channel_emulator_data_corrupt (emul, emul->srcpad, buf);
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_channel_emulator_debug, "channelemulator",
      0, "Network channel emulator plugin");

  return gst_element_register (plugin, "channelemulator", GST_RANK_NONE,
      GST_TYPE_CHANNEL_EMULATOR);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    channelemulator,
    "Network channel emulator",
    plugin_init,
    VERSION, "LGPL", "Samsung Electronics Co", "http://www.samsung.com")
