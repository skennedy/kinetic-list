/*
 * mx-scroll-view.h: Container with scroll-bars
 *
 * Copyright 2008 OpenedHand
 * Copyright 2009 Intel Corporation.
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
 * Written by: Chris Lord <chris@openedhand.com>
 * Port to Mx by: Robert Staudinger <robsta@openedhand.com>
 *
 */

/**
 * SECTION:mx-scroll-view
 * @short_description: a container for scrollable children
 *
 * #KineticFrame is a single child container for actors that implement
 * #MxScrollable. It provides scrollbars around the edge of the child to
 * allow the user to move around the scrollable area.
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

    PROP_HSCROLL,
    PROP_VSCROLL,
    PROP_MOUSE_SCROLL
};

static void
kinetic_frame_get_property (GObject    *object,
        guint       property_id,
        GValue     *value,
        GParamSpec *pspec)
{
    /*
       KineticFramePrivate *priv = ((KineticFrame *) object)->priv;

       switch (property_id)
       {
       case PROP_HSCROLL:
       g_value_set_object (value, priv->hscroll);
       break;
       case PROP_VSCROLL:
       g_value_set_object (value, priv->vscroll);
       break;
       case PROP_MOUSE_SCROLL:
       g_value_set_boolean (value, priv->mouse_scroll);
       break;
       default:
       G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
       }
     */
}

static void
kinetic_frame_set_property (GObject      *object,
        guint         property_id,
        const GValue *value,
        GParamSpec   *pspec)
{
    switch (property_id)
    {
        /*
           case PROP_MOUSE_SCROLL:
           kinetic_frame_set_mouse_scrolling ((KineticFrame *) object,
           g_value_get_boolean (value));
         */
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
#if 0
static void
kinetic_frame_paint (ClutterActor *actor)
{
    //KineticFramePrivate *priv = KINETIC_FRAME (actor)->priv;

    /* MxBin will paint the child */
    CLUTTER_ACTOR_CLASS (kinetic_frame_parent_class)->paint (actor);

    /* paint our custom children */
    /*
       if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
       clutter_actor_paint (priv->hscroll);
       if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
       clutter_actor_paint (priv->vscroll);
     */
}

static void
kinetic_frame_pick (ClutterActor       *actor,
        const ClutterColor *color)
{
    //KineticFramePrivate *priv = KINETIC_FRAME (actor)->priv;

    /* Chain up so we get a bounding box pained (if we are reactive) */
    CLUTTER_ACTOR_CLASS (kinetic_frame_parent_class)->pick (actor, color);

    /* paint our custom children */
    /*
       if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
       clutter_actor_paint (priv->hscroll);
       if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
       clutter_actor_paint (priv->vscroll);
     */
}


static void
kinetic_frame_get_preferred_width (ClutterActor *actor,
        gfloat        for_height,
        gfloat       *min_width_p,
        gfloat       *natural_width_p)
{
    MxPadding padding;
    //guint xthickness;

    KineticFramePrivate *priv = KINETIC_FRAME (actor)->priv;

    if (!priv->child)
        return;

    mx_widget_get_padding (MX_WIDGET (actor), &padding);
    /*
       mx_stylable_get (MX_STYLABLE (actor),
       "scrollbar-width", &xthickness,
       NULL);
     */

    /* Our natural width is the natural width of the child */
    clutter_actor_get_preferred_width (priv->child,
            for_height,
            NULL,
            natural_width_p);

    /* Add space for the scroll-bar if we can determine it will be necessary */
    /*
       if ((for_height >= 0) && natural_width_p)
       {
       gfloat natural_height;

       clutter_actor_get_preferred_height (priv->child, -1.0,
       NULL,
       &natural_height);
       if (for_height < natural_height)
     *natural_width_p += xthickness;
     }
     */

    /* Add space for padding */
    if (min_width_p)
        *min_width_p = padding.left + padding.right;

    if (natural_width_p)
        *natural_width_p += padding.left + padding.right;
}

static void
kinetic_frame_get_preferred_height (ClutterActor *actor,
        gfloat        for_width,
        gfloat       *min_height_p,
        gfloat       *natural_height_p)
{
    MxPadding padding;
    //guint ythickness;

    KineticFramePrivate *priv = KINETIC_FRAME (actor)->priv;

    if (!priv->child)
        return;

    mx_widget_get_padding (MX_WIDGET (actor), &padding);
    /*
       mx_stylable_get (MX_STYLABLE (actor),
       "scrollbar-height", &ythickness,
       NULL);
     */

    /* Our natural height is the natural height of the child */
    clutter_actor_get_preferred_height (priv->child,
            for_width,
            NULL,
            natural_height_p);

    /* Add space for the scroll-bar if we can determine it will be necessary */
    /*
       if ((for_width >= 0) && natural_height_p)
       {
       gfloat natural_width;

       clutter_actor_get_preferred_width (priv->child, -1.0,
       NULL,
       &natural_width);
       if (for_width < natural_width)
     *natural_height_p += ythickness;
     }
     */

    /* Add space for padding */
    if (min_height_p)
        *min_height_p = padding.top + padding.bottom;

    if (natural_height_p)
        *natural_height_p += padding.top + padding.bottom;
}

#endif
static void
kinetic_frame_allocate (ClutterActor          *actor,
        const ClutterActorBox *box,
        ClutterAllocationFlags flags)
{
    CLUTTER_ACTOR_CLASS(kinetic_frame_parent_class)->allocate(actor, box, flags);
#if 0
    MxPadding padding;
    ClutterActorBox child_box;
    ClutterActorClass *parent_parent_class;
    gfloat avail_width, avail_height, sb_width, sb_height;

    KineticFramePrivate *priv = KINETIC_FRAME (actor)->priv;

    /* Chain up to the parent's parent class
     *
     * We do this because we do not want MxBin to allocate the child, as we
     * give it a different allocation later, depending on whether the scrollbars
     * are visible
     */
    parent_parent_class
        = g_type_class_peek_parent (kinetic_frame_parent_class);

    CLUTTER_ACTOR_CLASS (parent_parent_class)->
        allocate (actor, box, flags);


    mx_widget_get_padding (MX_WIDGET (actor), &padding);

    avail_width = (box->x2 - box->x1) - padding.left - padding.right;
    avail_height = (box->y2 - box->y1) - padding.top - padding.bottom;

    mx_stylable_get (MX_STYLABLE (actor),
            "scrollbar-width", &sb_width,
            "scrollbar-height", &sb_height,
            NULL);
    sb_width = 28;
    sb_height = 28;

    if (!CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
        sb_width = 0;

    if (!CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
        sb_height = 0;

    /* Vertical scrollbar */
    if (CLUTTER_ACTOR_IS_VISIBLE (priv->vscroll))
    {
        child_box.x1 = avail_width - sb_width;
        child_box.y1 = padding.top;
        child_box.x2 = avail_width;
        child_box.y2 = child_box.y1 + avail_height - sb_height;

        clutter_actor_allocate (priv->vscroll, &child_box, flags);
    }

    /* Horizontal scrollbar */
    if (CLUTTER_ACTOR_IS_VISIBLE (priv->hscroll))
    {
        child_box.x1 = padding.left;
        child_box.x2 = child_box.x1 + avail_width - sb_width;
        child_box.y1 = avail_height - sb_height;
        child_box.y2 = avail_height;

        clutter_actor_allocate (priv->hscroll, &child_box, flags);
    }


    /* Child */
    child_box.x1 = padding.left;
    child_box.x2 = avail_width - sb_width;
    child_box.y1 = padding.top;
    child_box.y2 = avail_height - sb_height;

    if (priv->child)
        clutter_actor_allocate (priv->child, &child_box, flags);
#endif
}

static void
kinetic_frame_style_changed (MxWidget *widget)
{
    /*
       KineticFramePrivate *priv = KINETIC_FRAME (widget)->priv;

       mx_stylable_changed ((MxStylable *) priv->hscroll);
       mx_stylable_changed ((MxStylable *) priv->vscroll);
     */
}

static gboolean
kinetic_frame_button_press_event (ClutterActor       *self,
        ClutterButtonEvent *event)
{
    KineticFramePrivate *priv = KINETIC_FRAME(self)->priv;

    if (event->button != 1) return FALSE;

    g_print("button press\n");

    priv->dragging = TRUE;
    if (priv->timeline) {
        clutter_timeline_stop(priv->timeline);
        priv->vel_x = priv->vel_y = 0;
        g_print("Stop kinetics\n");
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
new_frame_cb (ClutterTimeline *timeline,
              gint             elapsed_msecs,
              ClutterActor    *actor)
{
    KineticFramePrivate *priv = KINETIC_FRAME(actor)->priv;
    guint delta = clutter_timeline_get_delta(timeline);

    if (priv->vel_x != 0) {
        gfloat dx = priv->vel_x * delta;
        if (ABS(priv->vel_y) < VELOCITY_CUT_OFF) {
            priv->vel_x = 0;
        } else {
            mx_adjustment_set_value(
                priv->hadjust, mx_adjustment_get_value(priv->hadjust) + dx);
            priv->vel_x *= 1.0f - 1.0f / delta * FRICTION_COEFFICIENT;
        }
    }

    if (priv->vel_y != 0) {
        gfloat dy = priv->vel_y * delta;
        if (ABS(priv->vel_y) < VELOCITY_CUT_OFF) {
            priv->vel_y = 0;
        } else {
            mx_adjustment_set_value(
                priv->vadjust, mx_adjustment_get_value(priv->vadjust) + dy);
            priv->vel_y *= 1.0f - 1.0f / delta * FRICTION_COEFFICIENT;
        }
    }

    if (priv->vel_x == 0 && priv->vel_y == 0) {
        clutter_timeline_stop(timeline);
        g_print("Stop kinetics\n");
    }
}

static gboolean
kinetic_frame_button_release_event (ClutterActor       *self,
        ClutterButtonEvent *event)
{
    KineticFramePrivate *priv = KINETIC_FRAME(self)->priv;

    if (event->button != 1) return FALSE;

    g_print("button release   %f, %f\n", priv->vel_x, priv->vel_y);

    if (ABS(priv->vel_x) > VELOCITY_START_MIN ||
        ABS(priv->vel_y) > VELOCITY_START_MIN) {
        g_print("Start kinetics\n");

        if (!priv->timeline) {
            priv->timeline = clutter_timeline_new(1000);
            clutter_timeline_set_loop (priv->timeline, TRUE);
            g_signal_connect(priv->timeline, "new-frame", G_CALLBACK (new_frame_cb), self);
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
    g_print("capture\n");
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
    //GParamSpec *pspec;
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    ClutterActorClass *actor_class = CLUTTER_ACTOR_CLASS (klass);

    g_type_class_add_private (klass, sizeof (KineticFramePrivate));

    object_class->get_property = kinetic_frame_get_property;
    object_class->set_property = kinetic_frame_set_property;
    object_class->dispose= kinetic_frame_dispose;
    object_class->finalize = kinetic_frame_finalize;

    //actor_class->paint = kinetic_frame_paint;
    //actor_class->pick = kinetic_frame_pick;
    //actor_class->get_preferred_width = kinetic_frame_get_preferred_width;
    //actor_class->get_preferred_height = kinetic_frame_get_preferred_height;
    actor_class->allocate = kinetic_frame_allocate;
    actor_class->captured_event = kinetic_frame_captured_event;
    actor_class->button_press_event = kinetic_frame_button_press_event;
    actor_class->button_release_event = kinetic_frame_button_release_event;
    actor_class->motion_event = kinetic_frame_motion_event;

    /*
       g_object_class_install_property (object_class,
       PROP_HSCROLL,
       g_param_spec_object ("hscroll",
       "MxScrollBar",
       "Horizontal scroll indicator",
       MX_TYPE_SCROLL_BAR,
       G_PARAM_READABLE));

       g_object_class_install_property (object_class,
       PROP_VSCROLL,
       g_param_spec_object ("vscroll",
       "MxScrollBar",
       "Vertical scroll indicator",
       MX_TYPE_SCROLL_BAR,
       G_PARAM_READABLE));

       pspec = g_param_spec_boolean ("enable-mouse-scrolling",
       "Enable Mouse Scrolling",
       "Enable automatic mouse wheel scrolling",
       TRUE,
       G_PARAM_READWRITE);
       g_object_class_install_property (object_class,
       PROP_MOUSE_SCROLL,
       pspec);

     */

}

static void
mx_stylable_iface_init (MxStylableIface *iface)
{
    /*
       static gboolean is_initialized = FALSE;

       if (!is_initialized)
       {
       GParamSpec *pspec;

       is_initialized = TRUE;

       pspec = g_param_spec_uint ("scrollbar-width",
       "Vertical scroll-bar thickness",
       "Thickness of vertical scrollbar, in px",
       0, G_MAXUINT, 24,
       G_PARAM_READWRITE);
       mx_stylable_iface_install_property (iface, TYPE_KINETIC_FRAME, pspec);

       pspec = g_param_spec_uint ("scrollbar-height",
       "Horizontal scroll-bar thickness",
       "Thickness of horizontal scrollbar, in px",
       0, G_MAXUINT, 24,
       G_PARAM_READWRITE);
       mx_stylable_iface_install_property (iface, TYPE_KINETIC_FRAME, pspec);
       }
     */
}

static void
child_adjustment_changed_cb(MxAdjustment *adjustment, ClutterActor *self)
{
    gdouble lower, upper, step_inc, page_inc, page_size;
    mx_adjustment_get_values(adjustment, NULL, &lower, &upper, &step_inc, &page_inc, &page_size);
    g_print("adjustment %p changed %lf -> %lf  step_inc:%lf page_inc:%lf page_size:%lf\n", adjustment, lower, upper, step_inc, page_inc, page_size);
    clutter_actor_queue_relayout(self);
}

static void
child_hadjustment_notify_cb (GObject *gobject,
        GParamSpec *arg1,
        gpointer    user_data)
{
    g_print("hadjustment changed\n");
    MxAdjustment *hadjust;
    ClutterActor *child = CLUTTER_ACTOR (gobject);
    ClutterActor *self = CLUTTER_ACTOR (user_data);
    KineticFramePrivate *priv = KINETIC_FRAME (user_data)->priv;

    mx_scrollable_get_adjustments (MX_SCROLLABLE (child), &hadjust, NULL);

    if (hadjust != priv->hadjust) {
        if (priv->hadjust) {
            g_signal_handlers_disconnect_by_func(priv->hadjust,
                    child_adjustment_changed_cb,
                    self);
            g_object_unref(priv->hadjust);
        }
        if (hadjust) {
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
    g_print("vadjustment changed\n");
    MxAdjustment *vadjust;
    ClutterActor *child = CLUTTER_ACTOR (gobject);
    ClutterActor *self = CLUTTER_ACTOR (user_data);
    KineticFramePrivate *priv = KINETIC_FRAME (user_data)->priv;

    mx_scrollable_get_adjustments (MX_SCROLLABLE (child), NULL, &vadjust);

    if (vadjust != priv->vadjust) {
        if (priv->vadjust) {
            g_signal_handlers_disconnect_by_func(priv->vadjust,
                    child_adjustment_changed_cb,
                    self);
            g_object_unref(priv->vadjust);
        }
        if (vadjust) {
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

