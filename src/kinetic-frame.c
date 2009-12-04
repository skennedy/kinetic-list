/*
 * kinetic-frame.h: Container that allows kinetic scrolling
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
 *
 * Written by: Stephen Kennedy <stephen.kennedy@qwondo.com>
 *
 */

/**
 * SECTION:kinetic-frame
 * @short_description: a container that allows kinetic scrolling
 *
 * #KineticFrame is a single child container for actors that implement
 * #MxScrollable. It allows the user to kinetic scroll the scrollable area
 * using dragging mouse movements
 */

#include "kinetic-frame.h"
#include <clutter/clutter.h>

static void clutter_container_iface_init (ClutterContainerIface *iface);
static void mx_stylable_iface_init (MxStylableIface *iface);

static ClutterContainerIface *kinetic_frame_parent_iface = NULL;

G_DEFINE_TYPE_WITH_CODE (KineticFrame, kinetic_frame, MX_TYPE_BIN,
                         G_IMPLEMENT_INTERFACE (CLUTTER_TYPE_CONTAINER,
                                                clutter_container_iface_init)
                         G_IMPLEMENT_INTERFACE (MX_TYPE_STYLABLE,
                                                mx_stylable_iface_init))

#define KINETIC_FRAME_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
                                                               TYPE_KINETIC_FRAME, \
                                                               KineticFramePrivate))

/* Constants for kinetics */
#define VELOCITY_START_MIN      (15.0f/1000) // pixels per msec
#define VELOCITY_CUT_OFF        (30.0f/1000)
#define FRICTION_COEFFICIENT    (0.5)    
#define K_VALUE                 (3)

struct _KineticFramePrivate
{
    /* a pointer to the child; this is actually stored
     * inside MxBin:child, but we keep it to avoid
     * calling mx_bin_get_child() every time we need it
     */
    ClutterActor       *child;

    /* The adjustments from the scrollable child actor */
    MxAdjustment       *hadjust;
    MxAdjustment       *vadjust;

    gboolean            dragging;
    gfloat              last_x, last_y;
    guint32             last_t;
    gfloat              vel_x, vel_y;

    ClutterTimeline    *timeline;
};

enum {
    PROP_0,

    /* Property enum values here */
};

static void
kinetic_frame_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
    switch (property_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
kinetic_frame_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
    switch (property_id)
    {
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
kinetic_frame_dispose (GObject *object)
{
    KineticFramePrivate *priv = KINETIC_FRAME (object)->priv;

    if (priv->hadjust)
    {
        g_object_unref(priv->hadjust);
        priv->hadjust = NULL;
    }

    if (priv->vadjust)
    {
        g_object_unref(priv->vadjust);
        priv->vadjust = NULL;
    }

    if (priv->timeline)
    {
        g_object_unref(priv->timeline);
        priv->timeline = NULL;
    }

    /* Chaining up will remove the child actor */
    G_OBJECT_CLASS (kinetic_frame_parent_class)->dispose (object);
}

static void
kinetic_frame_finalize (GObject *object)
{
    G_OBJECT_CLASS (kinetic_frame_parent_class)->finalize (object);
}

static void
kinetic_frame_style_changed (MxWidget *widget)
{
    // Do something ?
}

static gboolean
kinetic_frame_button_press_event (ClutterActor       *self,
                                  ClutterButtonEvent *event)
{
    KineticFramePrivate *priv = KINETIC_FRAME(self)->priv;

    if (event->button != 1) return FALSE;

    priv->dragging = TRUE;

    if (priv->timeline) 
    {
        clutter_timeline_stop(priv->timeline);
        priv->vel_x = priv->vel_y = 0;
    }

    priv->last_x = event->x;
    priv->last_y = event->y;

    clutter_actor_transform_stage_point (self,
                                         priv->last_x, priv->last_y,
                                         &priv->last_x, &priv->last_y);
    return TRUE;
}

static gboolean
kinetic_frame_motion_event (ClutterActor        *self,
                            ClutterMotionEvent  *event)
{
    KineticFramePrivate *priv = KINETIC_FRAME(self)->priv;
    gfloat new_x, new_y, dx, dy, new_vel_x, new_vel_y;

    if (!priv->dragging) return FALSE;

    new_x = event->x;
    new_y = event->y;

    clutter_actor_transform_stage_point(self,
                                        new_x, new_y,
                                        &new_x, &new_y);

    /* Calculate motion displacement */
    dx = priv->last_x - new_x;
    dy = priv->last_y - new_y;

    /* Update scroll adjustment */
    mx_adjustment_set_value(priv->hadjust, 
                            mx_adjustment_get_value(priv->hadjust) + dx);
    mx_adjustment_set_value(priv->vadjust, 
                            mx_adjustment_get_value(priv->vadjust) + dy);

    /* Calculate new velocity estimates from displacements */
    new_vel_x = dx / (event->time - priv->last_t);
    new_vel_y = dy / (event->time - priv->last_t);

    /* Update velocity using incremental update rule */
    priv->vel_x = priv->vel_x + 1.0f / (K_VALUE + 1) * (new_vel_x - priv->vel_x);
    priv->vel_y = priv->vel_y + 1.0f / (K_VALUE + 1) * (new_vel_y - priv->vel_y);

    priv->last_x = new_x;
    priv->last_y = new_y;
    priv->last_t = event->time;

    return TRUE;
}

static void
timeline_new_frame_cb (ClutterTimeline *timeline,
                       gint             elapsed_msecs,
                       ClutterActor    *actor)
{
    KineticFramePrivate *priv = KINETIC_FRAME(actor)->priv;
    guint delta = clutter_timeline_get_delta(timeline);

    if (priv->vel_x != 0) 
    {
        if (ABS(priv->vel_y) < VELOCITY_CUT_OFF) 
        {
            priv->vel_x = 0;
        } 
        else 
        {
            /* Update scroll adjustment */
            gfloat dx = priv->vel_x * delta;
            mx_adjustment_set_value(
                priv->hadjust, mx_adjustment_get_value(priv->hadjust) + dx);
            /* Slow down based on friction and time passed */
            priv->vel_x *= 1.0f - 1.0f / delta * FRICTION_COEFFICIENT;
        }
    }

    if (priv->vel_y != 0) 
    {
        if (ABS(priv->vel_y) < VELOCITY_CUT_OFF) 
        {
            priv->vel_y = 0;
        } 
        else 
        {
            /* Update scroll adjustment */
            gfloat dy = priv->vel_y * delta;
            mx_adjustment_set_value(
                priv->vadjust, mx_adjustment_get_value(priv->vadjust) + dy);
            /* Slow down based on friction and time passed */
            priv->vel_y *= 1.0f - 1.0f / delta * FRICTION_COEFFICIENT;
        }
    }

    /* Stop timeline if no more movement required */
    if (priv->vel_x == 0 && priv->vel_y == 0) 
    {
        clutter_timeline_stop(timeline);
    }
}

static gboolean
kinetic_frame_button_release_event (ClutterActor       *self,
                                    ClutterButtonEvent *event)
{
    KineticFramePrivate *priv = KINETIC_FRAME(self)->priv;

    if (event->button != 1) return FALSE;

    if (ABS(priv->vel_x) > VELOCITY_START_MIN ||
        ABS(priv->vel_y) > VELOCITY_START_MIN) 
    {
        if (!priv->timeline) 
        {
            priv->timeline = clutter_timeline_new(1000);
            clutter_timeline_set_loop (priv->timeline, TRUE);
            g_signal_connect(priv->timeline, "new-frame", G_CALLBACK (timeline_new_frame_cb), self);
        }
        clutter_timeline_start(priv->timeline);
    }

    priv->dragging = FALSE;

    return TRUE;
}

static gboolean
kinetic_frame_captured_event (ClutterActor       *self,
                              ClutterEvent *event)
{
    /* Intercept appropriate events and pass to handlers */
    switch (event->type) 
    {
    case CLUTTER_MOTION:
        return kinetic_frame_motion_event(
            self, (ClutterMotionEvent*)event );
    case CLUTTER_BUTTON_PRESS:
        return kinetic_frame_button_press_event(
            self, (ClutterButtonEvent*)event );
    case CLUTTER_BUTTON_RELEASE:
        return kinetic_frame_button_release_event(
            self, (ClutterButtonEvent*)event );
    default:
        return FALSE;
    }
    return FALSE;
}


static void
kinetic_frame_class_init (KineticFrameClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KineticFramePrivate));

    object_class->get_property = kinetic_frame_get_property;
    object_class->set_property = kinetic_frame_set_property;
    object_class->dispose= kinetic_frame_dispose;
    object_class->finalize = kinetic_frame_finalize;

    actor_class->captured_event = kinetic_frame_captured_event;
    actor_class->button_press_event = kinetic_frame_button_press_event;
    actor_class->button_release_event = kinetic_frame_button_release_event;
    actor_class->motion_event = kinetic_frame_motion_event;
}

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
    /* Setup style properties here */

}

static void
child_adjustment_changed_cb(MxAdjustment *adjustment, ClutterActor *self)
{
    /* TODO: is this required? */
    clutter_actor_queue_relayout(self);
}

static void
child_hadjustment_notify_cb (GObject *gobject,
                             GParamSpec *arg1,
                             gpointer    user_data)
{
    MxAdjustment *hadjust;
    ClutterActor *child = CLUTTER_ACTOR (gobject);
    ClutterActor *self = CLUTTER_ACTOR (user_data);
    KineticFramePrivate *priv = KINETIC_FRAME (user_data)->priv;

    mx_scrollable_get_adjustments (MX_SCROLLABLE (child), &hadjust, NULL);

    if (hadjust != priv->hadjust) 
    {
        if (priv->hadjust) 
        {
            g_signal_handlers_disconnect_by_func(priv->hadjust,
                                                 child_adjustment_changed_cb,
                                                 self);
            g_object_unref(priv->hadjust);
        }

        if (hadjust) 
        {
            g_object_ref(hadjust);
            g_signal_connect (hadjust, "changed", 
                              G_CALLBACK(child_adjustment_changed_cb), self);
            child_adjustment_changed_cb (hadjust, self);
        }
        priv->hadjust = hadjust;
    } 
}

static void
child_vadjustment_notify_cb (GObject    *gobject,
                             GParamSpec *arg1,
                             gpointer    user_data)
{
    MxAdjustment *vadjust;
    ClutterActor *child = CLUTTER_ACTOR (gobject);
    ClutterActor *self = CLUTTER_ACTOR (user_data);
    KineticFramePrivate *priv = KINETIC_FRAME (user_data)->priv;

    mx_scrollable_get_adjustments (MX_SCROLLABLE (child), NULL, &vadjust);

    if (vadjust != priv->vadjust) 
    {
        if (priv->vadjust) 
        {
            g_signal_handlers_disconnect_by_func(priv->vadjust,
                                                 child_adjustment_changed_cb,
                                                 self);
            g_object_unref(priv->vadjust);
        }

        if (vadjust) 
        {
            g_object_ref(vadjust);
            g_signal_connect (vadjust, "changed", 
                              G_CALLBACK(child_adjustment_changed_cb), self);
            child_adjustment_changed_cb (vadjust, self);
        }
        priv->vadjust = vadjust;
    } 
}

static void
kinetic_frame_init (KineticFrame *self)
{
    KineticFramePrivate *priv = self->priv = KINETIC_FRAME_PRIVATE (self);

    priv->dragging = FALSE;

    g_object_set(self, 
                 "reactive", TRUE, "clip-to-allocation", TRUE, NULL);
    g_signal_connect (self, "style-changed",
                      G_CALLBACK (kinetic_frame_style_changed), NULL);

}

static void
kinetic_frame_add (ClutterContainer *container,
                   ClutterActor     *actor)
{
    KineticFrame *self = KINETIC_FRAME (container);
    KineticFramePrivate *priv = self->priv;

    if (MX_IS_SCROLLABLE (actor))
    {
        priv->child = actor;

        /* chain up to MxBin::add() */
        kinetic_frame_parent_iface->add (container, actor);

        /* Get adjustments for scroll-bars */
        g_signal_connect (actor, "notify::hadjustment",
                          G_CALLBACK (child_hadjustment_notify_cb),
                          container);
        g_signal_connect (actor, "notify::vadjustment",
                          G_CALLBACK (child_vadjustment_notify_cb),
                          container);
        child_hadjustment_notify_cb (G_OBJECT (actor), NULL, container);
        child_vadjustment_notify_cb (G_OBJECT (actor), NULL, container);
    }
    else
    {
        g_warning ("Attempting to add an actor of type %s to "
                   "a KineticFrame, but the actor does "
                   "not implement MxScrollable.",
                   g_type_name (G_OBJECT_TYPE (actor)));
    }
}

static void
kinetic_frame_remove (ClutterContainer *container,
                      ClutterActor     *actor)
{
    KineticFramePrivate *priv = KINETIC_FRAME (container)->priv;

    if (actor == priv->child)
    {
        g_object_ref (priv->child);

        /* chain up to MxBin::remove() */
        kinetic_frame_parent_iface->remove (container, actor);

        g_signal_handlers_disconnect_by_func (priv->child,
                                              child_hadjustment_notify_cb,
                                              container);
        g_signal_handlers_disconnect_by_func (priv->child,
                                              child_vadjustment_notify_cb,
                                              container);
        mx_scrollable_set_adjustments ((MxScrollable*) priv->child, NULL, NULL);

        g_object_unref (priv->child);
        priv->child = NULL;
    }
}

static void
clutter_container_iface_init (ClutterContainerIface *iface)
{
    /* store a pointer to the MxBin implementation of
     * ClutterContainer so that we can chain up when
     * overriding the methods
     */
    kinetic_frame_parent_iface = g_type_interface_peek_parent (iface);

    iface->add = kinetic_frame_add;
    iface->remove = kinetic_frame_remove;
    //iface->foreach_with_internals = kinetic_frame_foreach_with_internals;
}

ClutterActor *
kinetic_frame_new (void)
{
    return g_object_new (TYPE_KINETIC_FRAME, NULL);
}

