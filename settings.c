/* Glib preferences & matched GTK+ dialog.
 * 
 * (c) 2011 by gatopeich, licensed under a Creative Commons Attribution 3.0
 * Unported License: http://creativecommons.org/licenses/by/3.0/
 * Briefly: Use it however suits you better and just give me due credit.
 *
 * TODO:
 * - Temp range selection.
 */

#include <gtk/gtk.h>

static gchar* pref_filename =  "gatotrayrc";
static GKeyFile* pref_file = NULL;

GdkColor fg_color, bg_color, iow_color;
GdkColor temp_min_color, temp_max_color, temp_gradient[100];
GdkColor freq_min_color, freq_max_color, freq_gradient[100];
typedef struct {
    const gchar* description;
    const gchar* preset;
    GdkColor* color;
} PrefColor;
PrefColor pref_colors[] = {
    { "Foreground", "black", &fg_color },
    { "Background", "white", &bg_color },
    { "I/O wait", "blue", &iow_color },
    { "Min frequency", "green", &freq_min_color },
    { "Max frequency", "red", &freq_max_color },
    { "Min temperature", "blue", &temp_min_color },
    { "Max temperature", "red", &temp_max_color },
};

gboolean pref_transparent = TRUE;
typedef struct {
    const gchar* description;
    gboolean* yesno;
} PrefBoolean;
PrefBoolean pref_booleans[] = {
    { "Transparent background", &pref_transparent },
};

gchar pref_command[] = "xterm -n 'gatotray: htop' -rv -geometry 100x32+40+40 top";
typedef struct {
    const gchar* description;
    gchar* command[255];
} PrefCommand;
// GCC was warning because I had ", &pref_command" as a second param
// causing the warning: missing braces around initializer [-Wmissing-braces]
// because the second parameter is not a const.
PrefCommand pref_commands[] = {
    { "Launch Command" },
};

void preferences_changed() {
    for(int i=0;i<100;i++) {
        freq_gradient[i].red = (freq_min_color.red*(99-i)+freq_max_color.red*i)/99;
        freq_gradient[i].green = (freq_min_color.green*(99-i)+freq_max_color.green*i)/99;
        freq_gradient[i].blue = (freq_min_color.blue*(99-i)+freq_max_color.blue*i)/99;
        temp_gradient[i].red = (temp_min_color.red*(99-i)+temp_max_color.red*i)/99;
        temp_gradient[i].green = (temp_min_color.green*(99-i)+temp_max_color.green*i)/99;
        temp_gradient[i].blue = (temp_min_color.blue*(99-i)+temp_max_color.blue*i)/99;
    }
}

void on_option_toggled(GtkToggleButton *togglebutton, PrefBoolean *b) {
    *b->yesno = gtk_toggle_button_get_active(togglebutton);
    preferences_changed();
}

void on_command_changed(GtkEntry *entry, PrefCommand *d) {
    // g_message("on_command_changed %s\n", g_strdup(gtk_entry_get_text(entry)));
    *d->command = g_strdup(gtk_entry_get_text(entry));
    preferences_changed();
}

void pref_init()
{
    g_assert(!pref_file);
    pref_file = g_key_file_new();
    gchar* path = g_strconcat(g_get_user_config_dir(), "/", pref_filename, NULL);
    if(g_key_file_load_from_file(pref_file, path, G_KEY_FILE_KEEP_COMMENTS, NULL))
        g_message("Loaded %s", path);
    g_free(path);

    for(PrefColor* c=pref_colors; c < pref_colors+G_N_ELEMENTS(pref_colors); c++)
    {
        gchar*
        value = g_key_file_get_string(pref_file, "Colors", c->description, NULL);
        if(!value)
            gdk_color_parse(c->preset, c->color);
        else {
            gdk_color_parse(value, c->color);
            g_free(value);
        }
    }
    for(PrefBoolean* b=pref_booleans; b < pref_booleans+G_N_ELEMENTS(pref_booleans); b++)
    {
        GError* gerror = NULL;
        gboolean
        value = g_key_file_get_boolean(pref_file, "Options", b->description, &gerror);
        if(!gerror)
            *b->yesno = value;
    }
    for(PrefCommand* d=pref_commands; d < pref_commands+G_N_ELEMENTS(pref_commands); d++)
    {
        gchar*
        value = g_key_file_get_string(pref_file, "Command", d->description, NULL);
        printf("Pref init with value %s\n", value);
        if(value)
            *d->command = value;
    }
    preferences_changed();
}

void pref_save()
{
    for(PrefColor* c=pref_colors; c<pref_colors+G_N_ELEMENTS(pref_colors); c++)
    {
        gchar *color = gdk_color_to_string(c->color);
        g_key_file_set_string(pref_file, "Colors", c->description, color);
        g_free(color);
    }
    for(PrefBoolean* b=pref_booleans; b<pref_booleans+G_N_ELEMENTS(pref_booleans); b++)
        g_key_file_set_boolean(pref_file, "Options", b->description, *b->yesno);
    for(PrefCommand* d=pref_commands; d<pref_commands+G_N_ELEMENTS(pref_commands); d++)
    {
        // g_message("Saving command value: %s\n", *d->command);
        g_key_file_set_string(pref_file, "Command", d->description, *d->command);
    }

    gchar* data = g_key_file_to_data(pref_file, NULL, NULL);
    gchar* path = g_strconcat(g_get_user_config_dir(), "/", pref_filename, NULL);
    g_file_set_contents(path, data, -1, NULL);
    g_free(path);
    g_free(data);
}

GtkWidget *pref_dialog = NULL;

void pref_response(GtkDialog *dialog, gint response_id, gpointer user_data)
{
    if(GTK_RESPONSE_ACCEPT == response_id) pref_save();
    gtk_widget_destroy(GTK_WIDGET(pref_dialog));
}

void pref_destroyed() { pref_dialog = NULL; }

void show_pref_dialog()
{
    if(pref_dialog) return;

    pref_dialog = gtk_dialog_new_with_buttons("gatotray Settings", NULL, 0
        , GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
    //~ gtk_window_set_icon_name(GTK_WINDOW(pref_dialog), GTK_STOCK_PREFERENCES);
    g_signal_connect(G_OBJECT(pref_dialog), "response",
                      G_CALLBACK(pref_response), NULL);
    g_signal_connect(G_OBJECT(pref_dialog), "destroy",
                      G_CALLBACK(pref_destroyed), NULL);

    GtkWidget *vb = gtk_dialog_get_content_area(GTK_DIALOG(pref_dialog));
    GtkWidget *hb = gtk_hbox_new(FALSE, 0);
    GtkWidget *wb = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(vb), hb);
    gtk_container_add(GTK_CONTAINER(vb), wb);

    GtkWidget *frame = gtk_frame_new("Colors");
    gtk_container_add(GTK_CONTAINER(hb), frame);
    vb = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(frame), vb);
    for(PrefColor* c=pref_colors; c < pref_colors+G_N_ELEMENTS(pref_colors); c++)
    {
        GtkWidget *hb = gtk_hbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(vb), hb);
        gtk_container_add(GTK_CONTAINER(hb), gtk_label_new(c->description));
        GtkWidget *cbutton = gtk_color_button_new_with_color(c->color);
        g_signal_connect(G_OBJECT(cbutton), "color-set",
                      G_CALLBACK(gtk_color_button_get_color), c->color);
        g_signal_connect_after(G_OBJECT(cbutton), "color-set",
                      G_CALLBACK(preferences_changed), c);
        gtk_box_pack_start(GTK_BOX(hb), cbutton, FALSE, FALSE, 0);
    }

    frame = gtk_frame_new("Options");
    gtk_container_add(GTK_CONTAINER(hb), frame);
    vb = gtk_vbox_new(FALSE,0);
    gtk_container_add(GTK_CONTAINER(frame), vb);
    for(PrefBoolean* b=pref_booleans; b < pref_booleans+G_N_ELEMENTS(pref_booleans); b++)
    {
        GtkWidget *cbutton = gtk_check_button_new_with_label(b->description);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cbutton), *b->yesno);
        g_signal_connect(G_OBJECT(cbutton), "toggled",
                         G_CALLBACK(on_option_toggled), b);
        gtk_box_pack_start(GTK_BOX(vb), cbutton, FALSE, FALSE, 0);
    }

    frame = gtk_frame_new("Command");
    gtk_container_add(GTK_CONTAINER(wb), frame);
    vb = gtk_hbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(frame), vb);
    for(PrefCommand* d=pref_commands; d<pref_commands+G_N_ELEMENTS(pref_commands); d++)
    {
        GtkWidget *centry = gtk_entry_new_with_max_length(255);
        gtk_entry_set_text(GTK_ENTRY(centry), *d->command);
        gtk_editable_set_editable(GTK_EDITABLE(centry), TRUE);
        gtk_entry_set_visibility(GTK_ENTRY(centry), TRUE);
        // g_message("Showing dialog with command value: %s\n", *d->command);
        g_signal_connect(G_OBJECT(centry), "changed",
                         G_CALLBACK(on_command_changed), d);
        gtk_box_pack_start(GTK_BOX(vb), centry, TRUE, TRUE, 0);
    }

    gtk_widget_show_all(GTK_WIDGET(pref_dialog));
}
