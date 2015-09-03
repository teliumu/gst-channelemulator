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

#ifndef __GST_CHANNEL_EMULATOR_H__
#define __GST_CHANNEL_EMULATOR_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_CHANNEL_EMULATOR \
  (gst_channel_emulator_get_type())
#define GST_CHANNEL_EMULATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CHANNEL_EMULATOR,GstChannelEmulator))
#define GST_CHANNEL_EMULATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CHANNEL_EMULATOR,GstChannelEmulatorClass))
#define GST_IS_CHANNEL_EMULATOR(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CHANNEL_EMULATOR))
#define GST_IS_CHANNEL_EMULATOR_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CHANNEL_EMULATOR))

typedef enum {
  GST_CHANNEL_EMULATOR_RANDOM_DROP,
  GST_CHANNEL_EMULATOR_BURST_DROP,
  GST_CHANNEL_EMULATOR_COMBINED_DROP,
  GST_CHANNEL_EMULATOR_MC_MODEL_DROP,
  GST_CHANNEL_EMULATOR_GE_MODEL_DROP,
  GST_CHANNEL_EMULATOR_USER_MODEL_DROP
} GstChannelEmulatorDropMode;

typedef enum {
  GST_CHANNEL_EMULATOR_TRANSMITED,
  GST_CHANNEL_EMULATOR_BURST_LOST,
  GST_CHANNEL_EMULATOR_BURST_TRANSMITED,
  GST_CHANNEL_EMULATOR_SINGLE_LOST
} GstChannelEmulatorMarkovChainState;

typedef struct _GstChannelEmulator      GstChannelEmulator;
typedef struct _GstChannelEmulatorClass GstChannelEmulatorClass;

/* Correlated Loss Generation models */
typedef struct _state state;

struct _state
{
  /* state of the Markov chain */
  GstChannelEmulatorMarkovChainState state;

  gdouble p1; /* p13 for Markov or p for GE */
  gdouble p2; /* p31 for Markov or r for GE */
  gdouble p3; /* p32 for Markov or h for GE */
  gdouble p4; /* p14 for Markov or 1-k for GE */
  gdouble p5; /* p23 for Markov */
};

struct _GstChannelEmulator
{
  GstElement element;

  GstPad *sinkpad, *srcpad;
  GRand *rand;
  guint32 rand_seed;
  state s;
  gint min_delay;
  gint max_delay;
  gfloat delay_probability;
  guint8 drop_mode;
  gfloat drop_probability;
  gfloat burst_probability;
  gboolean burst_start;
  guint current_burst;
  guint burst_length;
  guint dropped_packet;
  guint64 processed_packet;
  guint max_drop_packet;
  guint period;
  gfloat duplicate_probability;
  guint8 bit_mask;
  gfloat bit_lost_probability;
  gint bit_lost_byte_length;
  guint bit_lost_number;
  guint current_bit_lost;
  gfloat packet_corrupt_probability;
  guint packet_corrupt_bytes;
  guint packet_corrupt_number;
  guint current_packet_corrupt;
  gchar *user_drop_file;
  GstStructure *model_param;
  gchar *user_drop_model;
  guint model_len;
  guint processed_by_model;
};

struct _GstChannelEmulatorClass
{
  GstElementClass parent_class;
};

GType gst_channel_emulator_get_type (void);

G_END_DECLS

#endif /* __GST_CHANNEL_EMULATOR_H__ */
