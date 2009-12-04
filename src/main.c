#include <clutter/clutter.h>
#include <stdlib.h>
#include <glib/gprintf.h>
#include "kinetic-frame.h"

#define N (500)

int
main(int argc, char **argv)
{
    ClutterActor    *stage;
    ClutterActor    *kinetic;
    ClutterActor    *box_layout;
    ClutterColor     col = { 0, 0, 0, 255}; // black
    guint i;

    clutter_init (&argc, &argv);

    /* Get stage, and set it up */
    stage = clutter_stage_get_default ();
    clutter_stage_set_color(CLUTTER_STAGE(stage), &col);

    kinetic = kinetic_frame_new();
    clutter_container_add_actor(CLUTTER_CONTAINER(stage), kinetic);

    box_layout = g_object_new(MX_TYPE_BOX_LAYOUT,
                              "vertical", TRUE, 
                              NULL);
    clutter_actor_set_size(box_layout, 300, 450);
    clutter_container_add_actor(CLUTTER_CONTAINER(kinetic), box_layout);

    for (i = 0; i < N; i++) 
    {
        /*
        col.red = g_random_int_range(0, 256);
        col.green = g_random_int_range(0, 256);
        col.blue = g_random_int_range(0, 256);
        g_print("%d %d %d\n", col.red, col.green, col.blue);
        ClutterActor *rect = clutter_rectangle_new_with_color(&col);
        clutter_actor_set_size(rect, 300, 150);
        clutter_container_add_actor(CLUTTER_CONTAINER(box_layout), rect);
        clutter_actor_show(rect);
        */
        gchar label[10];
        g_sprintf(label, "Button %d", i);
        ClutterActor *button = mx_button_new_with_label(label);
        clutter_container_add_actor(CLUTTER_CONTAINER(box_layout), button);
    }

    clutter_actor_show(stage);

    clutter_main ();

    return EXIT_SUCCESS;
}
