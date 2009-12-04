/*
 * kinetic-frame.h: Container which scrolls contents kinetically
 *
 * Copyright 2009 Qwondo.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 * Boston, MA 02111-1307, USA.
 *
 * Written by: Stephen Kennedy <stephen.kennedy@qwondo.com>
 *
 */

#ifndef __KINETIC_FRAME_H__
#define __KINETIC_FRAME_H__

#include <mx/mx.h>

G_BEGIN_DECLS

#define TYPE_KINETIC_FRAME            (kinetic_frame_get_type())
#define KINETIC_FRAME(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_KINETIC_FRAME, KineticFrame))
#define IS_KINETIC_FRAME(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_KINETIC_FRAME))
#define KINETIC_FRAME_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_KINETIC_FRAME, KineticFrameClass))
#define IS_KINETIC_FRAME_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_KINETIC_FRAME))
#define KINETIC_FRAME_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_KINETIC_FRAME, KineticFrameClass))

typedef struct _KineticFrame          KineticFrame;
typedef struct _KineticFramePrivate   KineticFramePrivate;
typedef struct _KineticFrameClass     KineticFrameClass;

/**
 * KineticFrame:
 *
 * The contents of this structure are private and should only be accessed
 * through the public API.
 */
struct _KineticFrame
{
  /*< private >*/
  MxBin parent_instance;

  KineticFramePrivate *priv;
};

struct _KineticFrameClass
{
  MxBinClass parent_class;
};

GType kinetic_frame_get_type (void) G_GNUC_CONST;

ClutterActor *kinetic_frame_new (void);

G_END_DECLS

#endif /* __KINETIC_FRAME_H__ */
